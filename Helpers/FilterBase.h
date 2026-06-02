#pragma once

// SPDX-License-Identifier: ISC
// Copyright 2007-2026 Jeff McClintock.

#include <algorithm>
#include <cmath>
#include "Processor.h"

/*
#include "Helpers/FilterBase.h"

Advanced 'sleep' behaviour for filters - or any Processor that takes some time
to settle after its input goes quiet.

The basic theory of 'power saving' in GMPI is:

1.  Wait until the input is silent (not streaming).
2.  Watch until the output is also silent (or at least very quiet). Now you are
    ready to sleep.
3.  Output some zeros until the output buffer is all zeros (sends perfect
    silence to the next module).
4.  Set the output pin status to 'not-streaming'.

Once all input and output pins are silent, the SDK will automatically sleep the
module. FilterBase just helps with that: it watches the output signal, and only
once the output has settled to a steady (unchanging) value does it let the
module go to sleep.

* For step 1, implement isFilterSettling() and return 'true' if the input is silent.
* For step 2, implement getOutputPin() to tell the base class which output to watch.
* Steps 3 and 4 are automatic.

USAGE:

    struct MyFilter : public gmpi::FilterBase
    {
        AudioInPin pinSignal;
        AudioOutPin pinOutput;

        // Called to determine when the filter is settling. Typically: are all
        // inputs quiet (not streaming)?
        bool isFilterSettling() override
        {
            return !pinSignal.isStreaming();
        }

        // Lets the base class monitor the filter's output signal.
        AudioOutPin& getOutputPin() override
        {
            return pinOutput;
        }

        void onSetPins() override
        {
            // ... usual stuff first ...
            initSettling(); // must be LAST.
        }
    };

Optional - stability: under heavy modulation, recursive filters can become
unstable and emit noise. Call doStabilityCheck() as the FIRST thing in your
subProcess (it only calls your StabilityCheck() one time in 50, to save CPU),
then override StabilityCheck() to reset your filter state if it has overflowed:

    void subProcess(int sampleFrames)
    {
        doStabilityCheck(); // must be first
        // ...etc
    }

    void StabilityCheck() override
    {
        if (!std::isfinite(my_state)) // check for numeric overflow.
            my_state = 0.0f;          // reset the filter by zeroing its state.
    }
*/

namespace gmpi
{

class FilterBase : public Processor
{
protected:
	// The filter's normal process function, saved while subProcessSettling() runs.
	SubProcessPtr actualSubProcess{};

	int   stabilityCheckCounter = 0; // counts down to the next StabilityCheck().
	float static_output = {};        // the steady value the output settled to.

	// How many consecutive 'flat' output samples we require before declaring the
	// filter settled, and how many of those are still outstanding.
	static constexpr int historyCount = 32;
	int historyIdx = 0;

public:
	ReturnCode open(api::IUnknown* phost) override
	{
		auto r = Processor::open(phost);

		// Randomise stabilityCheckCounter so a bank of filters don't all run
		// their (relatively expensive) checks on the same sample.
		stabilityCheckCounter = host->getHandle() & 63;

		return r;
	}

	// Convenience: filters very commonly need the sample-rate.
	float getSampleRate() const
	{
		return host->getSampleRate();
	}

	// --- implement these in your filter -----------------------------------

	// Return true when no input is feeding the filter (so it will decay to silence).
	virtual bool isFilterSettling() = 0;

	// The output pin the base class watches for silence.
	virtual AudioOutPin& getOutputPin() = 0;

	// Override to detect and reset numeric overflow in your filter's state.
	virtual void StabilityCheck() {}

	// --- provided by the base class ---------------------------------------

	// Called once the output has stopped changing: switch to emitting a constant,
	// and tell downstream modules this output is no longer streaming (so the host
	// can sleep us). Override if your filter has more than one output to tidy up.
	virtual void OnFilterSettled()
	{
		setSubProcess(&FilterBase::subProcessStatic);

		// assuming the settle check is always made at the start of the block.
		getOutputPin().setStreaming(false, getBlockPosition());
	}

	// Provides a counter to check the filter periodically. Call this FIRST in
	// your subProcess. Calls your StabilityCheck() only one time in 50.
	inline void doStabilityCheck()
	{
		if (--stabilityCheckCounter < 0)
		{
			StabilityCheck();
			stabilityCheckCounter = 50;
		}
	}

	// Runs the real filter, then watches the tail of the output buffer. Once the
	// output hasn't moved for 'historyCount' samples the filter is settled.
	void subProcessSettling(int sampleFrames)
	{
		if (historyIdx <= 0)
		{
			setSubProcess(actualSubProcess); // restore real process (prevents recursion)
			OnFilterSettled();
			(this->*(getSubProcess()))(sampleFrames); // e.g. subProcessStatic()
			return;
		}

		(this->*(actualSubProcess))(sampleFrames);

		// the value the output is converging toward.
		static_output = *(getBuffer(getOutputPin()) + sampleFrames - 1);

		const int todo = (std::min)(historyIdx, sampleFrames);
		auto o = getBuffer(getOutputPin()) + sampleFrames - todo;
		for (int i = 0; i < todo; ++i)
		{
			constexpr float INSIGNIFICANT_SAMPLE = 0.000001f;
			if (std::fabs(static_output - *o) > INSIGNIFICANT_SAMPLE)
				historyIdx = historyCount; // still moving: restart the count.

			--historyIdx;
			++o;
		}
	}

	// The output has settled to a constant: repeat it until the host sleeps us.
	// (Assumes a single output; override OnFilterSettled() for anything fancier.)
	void subProcessStatic(int sampleFrames)
	{
		auto output = getBuffer(getOutputPin());
		for (int s = sampleFrames; s > 0; s--)
			*output++ = static_output;
	}

	// If the filter is settling, start monitoring the output for silence.
	// Call this LAST in your onSetPins().
	void initSettling()
	{
		if (isFilterSettling())
		{
			if (getSubProcess() != static_cast<SubProcessPtr>(&FilterBase::subProcessSettling))
			{
				actualSubProcess = getSubProcess();
				setSubProcess(&FilterBase::subProcessSettling);
			}

			historyIdx = historyCount;
		}
	}
};

} // namespace gmpi
