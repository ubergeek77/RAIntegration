#include "CppUnitTest.h"

#include "RA_Achievement.h"
#include "RA_UnitTestHelpers.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ra {
namespace data {
namespace tests {

TEST_CLASS(RA_Achievement_Tests)
{
public:
    class AchievementHarness : public Achievement
    {
    public:
        AchievementHarness() : Achievement(AchievementSetType::Core) {}

        void ParseTrigger(const char* pTrigger) { Achievement::ParseTrigger(pTrigger); }
    };

    static int GetHitCount(Achievement& ach)
    {
        int nHitCount = 0;
        for (size_t i = 0; i < ach.NumConditionGroups(); ++i)
        {
            for (size_t j = 0; j < ach.NumConditions(i); ++j)
                nHitCount += ach.GetConditionHitCount(i, j);
        }

        return nHitCount;
    }

    void AssertSetTest(Achievement& ach, bool bExpectedResult, bool bExpectedDirty, bool bExpectedReset)
    {
        int nTotalHitCountBefore = GetHitCount(ach);

        Assert::AreEqual(bExpectedResult, ach.Test(), L"Test");

        int nTotalHitCountAfter = GetHitCount(ach);

        Assert::AreEqual(bExpectedDirty, nTotalHitCountAfter != nTotalHitCountBefore, L"bDirtyCondition");
        Assert::AreEqual(bExpectedReset, nTotalHitCountAfter < nTotalHitCountBefore, L"bWasReset");
    }

    void AssertSerialize(const char* sSerialized, const char* sExpected = nullptr)
    {
        if (sExpected == nullptr)
            sExpected = sSerialized;

        const char* ptr = sSerialized;
        AchievementHarness ach;
        ach.ParseTrigger(ptr);

        std::string sReserialized = ach.CreateMemString();

        Assert::AreEqual(sExpected, sReserialized.c_str(), Widen(sExpected).c_str());
    }

    TEST_METHOD(TestSerialize)
    {
        AssertSerialize("0xH0001=18");
        AssertSerialize("0xH0001=18_0xH0002=52");
        AssertSerialize("0xH0001=18_0xH0002=52_0xL0004=4");

        AssertSerialize("0xH0001=18.2._R:0xH0002=50_P:0xL0004=4");

        AssertSerialize("A:0xH0001=0_B:0xL0002=0_0xL0004=14");

        AssertSerialize("0xH0001=18.1._R:0xH0000=1S0xH0002=52.1.S0xL0004=6.1._P:0xH0000=2");

        AssertSerialize("0xH0001=4294967295"); // INT_MAX (32-bit max)
    }

    TEST_METHOD(TestSimpleSets) // Only standard conditions, no alt groups
    {
        unsigned char memory[] = { 0x00, 0x12, 0x34, 0xAB, 0x56 };
        InitializeMemory(memory, 5);

        AchievementHarness ach;
        ach.ParseTrigger("0xH0001=18"); // one condition, true
        AssertSetTest(ach, true, true, false);
        Assert::AreEqual(1U, ach.GetConditionHitCount(0, 0));
        AssertSetTest(ach, true, true, false);
        Assert::AreEqual(2U, ach.GetConditionHitCount(0, 0));

        ach.ParseTrigger("0xH0001!=18"); // one condition, false
        AssertSetTest(ach, false, false, false);
        Assert::AreEqual(0U, ach.GetConditionHitCount(0, 0));

        ach.ParseTrigger("0xH0001=18_0xH0002=52"); // two conditions, true
        AssertSetTest(ach, true, true, false);
        Assert::AreEqual(1U, ach.GetConditionHitCount(0, 0));
        Assert::AreEqual(1U, ach.GetConditionHitCount(0, 1));

        ach.ParseTrigger("0xH0001=18_0xH0002>52"); // two conditions, false
        AssertSetTest(ach, false, true, false);
        Assert::AreEqual(1U, ach.GetConditionHitCount(0, 0));
        Assert::AreEqual(0U, ach.GetConditionHitCount(0, 1));

        ach.ParseTrigger("0xH0001=18_0xH0002=52_0xL0004=6"); // three conditions, true
        AssertSetTest(ach, true, true, false);
        Assert::AreEqual(1U, ach.GetConditionHitCount(0, 0));
        Assert::AreEqual(1U, ach.GetConditionHitCount(0, 1));
        Assert::AreEqual(1U, ach.GetConditionHitCount(0, 2));

        ach.ParseTrigger("0xH0001=16_0xH0002=52_0xL0004=6"); // three conditions, first false
        AssertSetTest(ach, false, true, false);
        Assert::AreEqual(0U, ach.GetConditionHitCount(0, 0));
        Assert::AreEqual(1U, ach.GetConditionHitCount(0, 1));
        Assert::AreEqual(1U, ach.GetConditionHitCount(0, 2));

        ach.ParseTrigger("0xH0001=18_0xH0002=50_0xL0004=6"); // three conditions, first false
        AssertSetTest(ach, false, true, false);
        Assert::AreEqual(1U, ach.GetConditionHitCount(0, 0));
        Assert::AreEqual(0U, ach.GetConditionHitCount(0, 1));
        Assert::AreEqual(1U, ach.GetConditionHitCount(0, 2));

        ach.ParseTrigger("0xH0001=18_0xH0002=52_0xL0004=4"); // three conditions, first false
        AssertSetTest(ach, false, true, false);
        Assert::AreEqual(1U, ach.GetConditionHitCount(0, 0));
        Assert::AreEqual(1U, ach.GetConditionHitCount(0, 1));
        Assert::AreEqual(0U, ach.GetConditionHitCount(0, 2));

        ach.ParseTrigger("0xH0001=16_0xH0002=50_0xL0004=4"); // three conditions, all false
        AssertSetTest(ach, false, false, false);
        Assert::AreEqual(0U, ach.GetConditionHitCount(0, 0));
        Assert::AreEqual(0U, ach.GetConditionHitCount(0, 1));
        Assert::AreEqual(0U, ach.GetConditionHitCount(0, 2));
    }

    TEST_METHOD(TestPauseIf)
    {
        unsigned char memory[] = { 0x00, 0x12, 0x34, 0xAB, 0x56 };
        InitializeMemory(memory, 5);

        AchievementHarness ach;
        ach.ParseTrigger("0xH0001=18_P:0xH0002=52_P:0xL0x0004=6");
        AssertSetTest(ach, false, true, false);
        Assert::AreEqual(0U, ach.GetConditionHitCount(0, 0));
        Assert::AreEqual(1U, ach.GetConditionHitCount(0, 1));
        Assert::AreEqual(0U, ach.GetConditionHitCount(0, 2)); // Also true, but processing stops on first PauseIf

        memory[2] = 0;
        AssertSetTest(ach, false, false, false);
        Assert::AreEqual(0U, ach.GetConditionHitCount(0, 0));
        Assert::AreEqual(0U, ach.GetConditionHitCount(0, 1)); // PauseIf goes to 0 when false
        Assert::AreEqual(1U, ach.GetConditionHitCount(0, 2)); // PauseIf stays at 1 when false

        memory[4] = 0;
        AssertSetTest(ach, true, false, false);
        Assert::AreEqual(1U, ach.GetConditionHitCount(0, 0));
        Assert::AreEqual(0U, ach.GetConditionHitCount(0, 1));
        Assert::AreEqual(0U, ach.GetConditionHitCount(0, 2)); // PauseIf goes to 0 when false
    }


    TEST_METHOD(TestPauseIfHitCountOne)
    {
        unsigned char memory[] = { 0x00, 0x12, 0x34, 0xAB, 0x56 };
        InitializeMemory(memory, 5);

        AchievementHarness ach;
        ach.ParseTrigger("0xH0001=18_P:0xH0002=52.1.");
        AssertSetTest(ach, false, true, false);
        Assert::AreEqual(0U, ach.GetConditionHitCount(0, 0));
        Assert::AreEqual(1U, ach.GetConditionHitCount(0, 1));

        memory[2] = 0;
        AssertSetTest(ach, false, false, false);
        Assert::AreEqual(0U, ach.GetConditionHitCount(0, 0));
        Assert::AreEqual(1U, ach.GetConditionHitCount(0, 1)); // PauseIf with HitCount doesn't automatically go back to 0
    }

    TEST_METHOD(TestPauseIfHitCountTwo)
    {
        unsigned char memory[] = { 0x00, 0x12, 0x34, 0xAB, 0x56 };
        InitializeMemory(memory, 5);

        AchievementHarness ach;
        ach.ParseTrigger("0xH0001=18_P:0xH0002=52.2.");
        AssertSetTest(ach, true, true, false);                        // PauseIf counter hasn't reached HitCount target, non-PauseIf condition still true
        Assert::AreEqual(1U, ach.GetConditionHitCount(0, 0));
        Assert::AreEqual(1U, ach.GetConditionHitCount(0, 1));

        AssertSetTest(ach, false, true, false);                        // PauseIf counter has reached HitCount target, non-PauseIf conditions ignored
        Assert::AreEqual(1U, ach.GetConditionHitCount(0, 0));
        Assert::AreEqual(2U, ach.GetConditionHitCount(0, 1));

        memory[2] = 0;
        AssertSetTest(ach, false, false, false);
        Assert::AreEqual(1U, ach.GetConditionHitCount(0, 0));
        Assert::AreEqual(2U, ach.GetConditionHitCount(0, 1)); // PauseIf with HitCount doesn't automatically go back to 0
    }

    TEST_METHOD(TestPauseIfHitReset)
    {
        unsigned char memory[] = { 0x00, 0x12, 0x34, 0xAB, 0x56 };
        InitializeMemory(memory, 5);

        AchievementHarness ach;
        ach.ParseTrigger("0xH0001=18_P:0xH0002=52.1._R:0xH0003=1SR:0xH0003=2");
        AssertSetTest(ach, false, true, false);                        // Trigger PauseIf, non-PauseIf conditions ignored
        Assert::AreEqual(0U, ach.GetConditionHitCount(0, 0));
        Assert::AreEqual(1U, ach.GetConditionHitCount(0, 1));
        Assert::AreEqual(0U, ach.GetConditionHitCount(0, 2));
        Assert::AreEqual(0U, ach.GetConditionHitCount(1, 0));

        memory[2] = 0;
        AssertSetTest(ach, false, false, false);
        Assert::AreEqual(0U, ach.GetConditionHitCount(0, 0));
        Assert::AreEqual(1U, ach.GetConditionHitCount(0, 1)); // PauseIf with HitCount doesn't automatically go back to 0
        Assert::AreEqual(0U, ach.GetConditionHitCount(0, 2));
        Assert::AreEqual(0U, ach.GetConditionHitCount(1, 0));

        memory[3] = 1;
        AssertSetTest(ach, false, false, false);                      // ResetIf in Paused group is ignored
        Assert::AreEqual(0U, ach.GetConditionHitCount(0, 0));
        Assert::AreEqual(1U, ach.GetConditionHitCount(0, 1));
        Assert::AreEqual(0U, ach.GetConditionHitCount(0, 2));
        Assert::AreEqual(0U, ach.GetConditionHitCount(1, 0));

        memory[3] = 2;
        AssertSetTest(ach, false, true, true);                        // ResetIf in alternate group is honored, PauseIf does not retrigger and non-PauseIf condition is true
        Assert::AreEqual(0U, ach.GetConditionHitCount(0, 0)); // ResetIf causes entire achievement to fail
        Assert::AreEqual(0U, ach.GetConditionHitCount(0, 1));
        Assert::AreEqual(0U, ach.GetConditionHitCount(0, 2));
        Assert::AreEqual(0U, ach.GetConditionHitCount(1, 0));

        memory[3] = 3;
        AssertSetTest(ach, true, true, false);                         // ResetIf no longer true, achievement allowed to trigger
        Assert::AreEqual(1U, ach.GetConditionHitCount(0, 0));
        Assert::AreEqual(0U, ach.GetConditionHitCount(0, 1));
        Assert::AreEqual(0U, ach.GetConditionHitCount(0, 2));
        Assert::AreEqual(0U, ach.GetConditionHitCount(1, 0));
    }

    TEST_METHOD(TestResetIf)
    {
        unsigned char memory[] = { 0x00, 0x12, 0x34, 0xAB, 0x56 };
        InitializeMemory(memory, 5);

        AchievementHarness ach;
        ach.ParseTrigger("0xH0001=18_R:0xH0002=50_R:0xL0x0004=4");
        AssertSetTest(ach, true, true, false);
        Assert::AreEqual(1U, ach.GetConditionHitCount(0, 0));
        Assert::AreEqual(0U, ach.GetConditionHitCount(0, 1));
        Assert::AreEqual(0U, ach.GetConditionHitCount(0, 2));

        AssertSetTest(ach, true, true, false);
        Assert::AreEqual(2U, ach.GetConditionHitCount(0, 0));
        Assert::AreEqual(0U, ach.GetConditionHitCount(0, 1));
        Assert::AreEqual(0U, ach.GetConditionHitCount(0, 2));

        memory[2] = 50;
        AssertSetTest(ach, false, true, true);
        Assert::AreEqual(0U, ach.GetConditionHitCount(0, 0));
        Assert::AreEqual(0U, ach.GetConditionHitCount(0, 1)); // True, but ResetIf also resets true marker
        Assert::AreEqual(0U, ach.GetConditionHitCount(0, 2));

        memory[4] = 0x54;
        AssertSetTest(ach, false, false, false);
        Assert::AreEqual(0U, ach.GetConditionHitCount(0, 0));
        Assert::AreEqual(0U, ach.GetConditionHitCount(0, 1)); // True, but ResetIf also resets true marker
        Assert::AreEqual(0U, ach.GetConditionHitCount(0, 2)); // Also true, but processing stop on first ResetIf

        memory[2] = 52;
        AssertSetTest(ach, false, false, false);
        Assert::AreEqual(0U, ach.GetConditionHitCount(0, 0));
        Assert::AreEqual(0U, ach.GetConditionHitCount(0, 1));
        Assert::AreEqual(0U, ach.GetConditionHitCount(0, 2)); // True, but ResetIf also resets true marker

        memory[4] = 0x56;
        AssertSetTest(ach, true, true, false);
        Assert::AreEqual(1U, ach.GetConditionHitCount(0, 0));
        Assert::AreEqual(0U, ach.GetConditionHitCount(0, 1));
        Assert::AreEqual(0U, ach.GetConditionHitCount(0, 2));
    }

    TEST_METHOD(TestHitCount)
    {
        unsigned char memory[] = { 0x00, 0x12, 0x34, 0xAB, 0x56 };
        InitializeMemory(memory, 5);

        AchievementHarness ach;
        ach.ParseTrigger("0xH0001=20(2)_0xH0002=52");

        AssertSetTest(ach, false, true, false);
        Assert::AreEqual(0U, ach.GetConditionHitCount(0, 0));
        Assert::AreEqual(1U, ach.GetConditionHitCount(0, 1));

        memory[1] = 20;
        AssertSetTest(ach, false, true, false);
        Assert::AreEqual(1U, ach.GetConditionHitCount(0, 0));
        Assert::AreEqual(2U, ach.GetConditionHitCount(0, 1));

        AssertSetTest(ach, true, true, false);
        Assert::AreEqual(2U, ach.GetConditionHitCount(0, 0));
        Assert::AreEqual(3U, ach.GetConditionHitCount(0, 1));

        AssertSetTest(ach, true, true, false);
        Assert::AreEqual(2U, ach.GetConditionHitCount(0, 0)); // hits stop increment once count it reached
        Assert::AreEqual(4U, ach.GetConditionHitCount(0, 1));
    }

    // verifies that ResetIf resets HitCounts
    TEST_METHOD(TestHitCountResetIf)
    {
        unsigned char memory[] = { 0x00, 0x12, 0x34, 0xAB, 0x56 };
        InitializeMemory(memory, 5);

        AchievementHarness ach;
        ach.ParseTrigger("0xH0001=18(2)_0xH0002=52_R:0xL0004=4");

        AssertSetTest(ach, false, true, false);
        Assert::AreEqual(1U, ach.GetConditionHitCount(0, 0));
        Assert::AreEqual(1U, ach.GetConditionHitCount(0, 1));

        AssertSetTest(ach, true, true, false);
        Assert::AreEqual(2U, ach.GetConditionHitCount(0, 0));
        Assert::AreEqual(2U, ach.GetConditionHitCount(0, 1));

        AssertSetTest(ach, true, true, false);
        Assert::AreEqual(2U, ach.GetConditionHitCount(0, 0));
        Assert::AreEqual(3U, ach.GetConditionHitCount(0, 1));

        memory[4] = 0x54;
        AssertSetTest(ach, false, true, true);
        Assert::AreEqual(0U, ach.GetConditionHitCount(0, 0));
        Assert::AreEqual(0U, ach.GetConditionHitCount(0, 1));

        memory[4] = 0x56;
        AssertSetTest(ach, false, true, false);
        Assert::AreEqual(1U, ach.GetConditionHitCount(0, 0));
        Assert::AreEqual(1U, ach.GetConditionHitCount(0, 1));

        AssertSetTest(ach, true, true, false);
        Assert::AreEqual(2U, ach.GetConditionHitCount(0, 0));
        Assert::AreEqual(2U, ach.GetConditionHitCount(0, 1));
    }

    // verifies that ResetIf with HitCount target only resets HitCounts when target is met
    TEST_METHOD(TestHitCountResetIfHitCount)
    {
        unsigned char memory[] = { 0x00, 0x12, 0x34, 0xAB, 0x56 };
        InitializeMemory(memory, 5);

        AchievementHarness ach;
        ach.ParseTrigger("0xH0001=18(2)_0xH0002=52_R:0xL0004=4.2.");

        AssertSetTest(ach, false, true, false); // HitCounts on conditions 1 and 2 are incremented
        AssertSetTest(ach, true, true, false);  // HitCounts on conditions 1 and 2 are incremented, cond 1 is now true so entire achievement is true
        AssertSetTest(ach, true, true, false);  // HitCount on condition 2 is incremented, cond 1 already met its target HitCount
        Assert::AreEqual(2U, ach.GetConditionHitCount(0, 0));
        Assert::AreEqual(3U, ach.GetConditionHitCount(0, 1));
        Assert::AreEqual(0U, ach.GetConditionHitCount(0, 2)); // ResetIf HitCount should still be 0

        memory[4] = 0x54;

        // first hit on ResetIf should not reset anything
        AssertSetTest(ach, true, true, false);
        Assert::AreEqual(2U, ach.GetConditionHitCount(0, 0)); // condition 1 stopped at it's HitCount target
        Assert::AreEqual(4U, ach.GetConditionHitCount(0, 1)); // condition 2 continues to increment
        Assert::AreEqual(1U, ach.GetConditionHitCount(0, 2)); // ResetIf HitCount should be 1

        // second hit on ResetIf should reset everything
        AssertSetTest(ach, false, true, true);
        Assert::AreEqual(0U, ach.GetConditionHitCount(0, 0));
        Assert::AreEqual(0U, ach.GetConditionHitCount(0, 1));
        Assert::AreEqual(0U, ach.GetConditionHitCount(0, 2)); // ResetIf HitCount should also be reset
    }

    // verifies that ResetIf works with AddHits
    TEST_METHOD(TestAddHitsResetIf)
    {
        unsigned char memory[] = { 0x00, 0x12, 0x34, 0xAB, 0x56 };
        InitializeMemory(memory, 5);

        AchievementHarness ach;
        ach.ParseTrigger("C:0xH0001=18_R:0xL0004=6(3)"); // never(repeated(3, byte(1) == 18 || low(4) == 6))
        AssertSetTest(ach, true, true, false); // result is true, no non-reset conditions
        Assert::AreEqual(1U, ach.GetConditionHitCount(0, 0));
        Assert::AreEqual(1U, ach.GetConditionHitCount(0, 1));

        AssertSetTest(ach, false, true, true); // total hits met (2 for each condition, only needed 3 total) (2 hits on condition 2 is not enough), result is always false if reset
        Assert::AreEqual(0U, ach.GetConditionHitCount(0, 0));
        Assert::AreEqual(0U, ach.GetConditionHitCount(0, 1));
    }

    // verifies that ResetIf HitCount(1) behaves like ResetIf without a HitCount
    TEST_METHOD(TestHitCountResetIfHitCountOne)
    {
        unsigned char memory[] = { 0x00, 0x12, 0x34, 0xAB, 0x56 };
        InitializeMemory(memory, 5);

        AchievementHarness ach;
        ach.ParseTrigger("0xH0001=18(2)_0xH0002=52_R:0xL0004=4.1.");

        AssertSetTest(ach, false, true, false); // HitCounts on conditions 1 and 2 are incremented
        AssertSetTest(ach, true, true, false);  // HitCounts on conditions 1 and 2 are incremented, cond 1 is now true so entire achievement is true
        AssertSetTest(ach, true, true, false);  // HitCount on condition 2 is incremented, cond 1 already met its target HitCount
        Assert::AreEqual(2U, ach.GetConditionHitCount(0, 0));
        Assert::AreEqual(3U, ach.GetConditionHitCount(0, 1));
        Assert::AreEqual(0U, ach.GetConditionHitCount(0, 2)); // ResetIf HitCount should still be 0

        memory[4] = 0x54;

        // ResetIf HitCount(1) should behave just like ResetIf without a HitCount - all items, including ResetIf should be reset.
        AssertSetTest(ach, false, true, true);
        Assert::AreEqual(0U, ach.GetConditionHitCount(0, 0));
        Assert::AreEqual(0U, ach.GetConditionHitCount(0, 1));
        Assert::AreEqual(0U, ach.GetConditionHitCount(0, 2)); // ResetIf HitCount should also be reset
    }

    // verifies that PauseIf stops HitCount processing
    TEST_METHOD(TestHitCountPauseIf)
    {
        unsigned char memory[] = { 0x00, 0x12, 0x34, 0xAB, 0x56 };
        InitializeMemory(memory, 5);

        AchievementHarness ach;
        ach.ParseTrigger("0xH0001=18(2)_0xH0002=52_P:0xL0004=4");

        AssertSetTest(ach, false, true, false);
        Assert::AreEqual(1U, ach.GetConditionHitCount(0, 0));
        Assert::AreEqual(1U, ach.GetConditionHitCount(0, 1));

        memory[4] = 0x54;
        AssertSetTest(ach, false, true, false);
        Assert::AreEqual(1U, ach.GetConditionHitCount(0, 0));
        Assert::AreEqual(1U, ach.GetConditionHitCount(0, 1));

        memory[4] = 0x56;
        AssertSetTest(ach, true, true, false);
        Assert::AreEqual(2U, ach.GetConditionHitCount(0, 0));
        Assert::AreEqual(2U, ach.GetConditionHitCount(0, 1));

        memory[4] = 0x54;
        AssertSetTest(ach, false, true, false);
        Assert::AreEqual(2U, ach.GetConditionHitCount(0, 0));
        Assert::AreEqual(2U, ach.GetConditionHitCount(0, 1));

        memory[4] = 0x56;
        AssertSetTest(ach, true, false, false);
        Assert::AreEqual(2U, ach.GetConditionHitCount(0, 0));
        Assert::AreEqual(3U, ach.GetConditionHitCount(0, 1));
    }

    // verifies that PauseIf prevents ResetIf processing
    TEST_METHOD(TestHitCountPauseIfResetIf)
    {
        unsigned char memory[] = { 0x00, 0x12, 0x34, 0xAB, 0x56 };
        InitializeMemory(memory, 5);

        AchievementHarness ach;
        ach.ParseTrigger("0xH0001=18(2)_R:0xH0002=50_P:0xL0004=4");

        AssertSetTest(ach, false, true, false);
        Assert::AreEqual(1U, ach.GetConditionHitCount(0, 0));

        memory[4] = 0x54; // pause
        AssertSetTest(ach, false, true, false);
        Assert::AreEqual(1U, ach.GetConditionHitCount(0, 0));

        memory[2] = 50; // reset (but still paused)
        AssertSetTest(ach, false, true, false);
        Assert::AreEqual(1U, ach.GetConditionHitCount(0, 0));

        memory[4] = 0x56; // unpause (still reset)
        AssertSetTest(ach, false, true, true);
        Assert::AreEqual(0U, ach.GetConditionHitCount(0, 0));

        memory[2] = 52; // unreset
        AssertSetTest(ach, false, true, false);
        Assert::AreEqual(1U, ach.GetConditionHitCount(0, 0));

        AssertSetTest(ach, true, true, false);
        Assert::AreEqual(2U, ach.GetConditionHitCount(0, 0));
    }

    TEST_METHOD(TestAddSource)
    {
        unsigned char memory[] = { 0x00, 0x12, 0x34, 0xAB, 0x56 };
        InitializeMemory(memory, 5);

        AchievementHarness ach;
        ach.ParseTrigger("A:0xH0001=0_0xH0002=22");
        AssertSetTest(ach, false, false, false);
        Assert::AreEqual(0U, ach.GetConditionHitCount(0, 0));
        Assert::AreEqual(0U, ach.GetConditionHitCount(0, 1));

        memory[2] = 4; // sum is correct
        AssertSetTest(ach, true, true, false);
        Assert::AreEqual(0U, ach.GetConditionHitCount(0, 0)); // AddSource condition does not have hit tracking
        Assert::AreEqual(1U, ach.GetConditionHitCount(0, 1));

        memory[1] = 0; // first condition is true, but not sum
        AssertSetTest(ach, false, false, false);
        Assert::AreEqual(0U, ach.GetConditionHitCount(0, 0)); // AddSource condition does not have hit tracking
        Assert::AreEqual(1U, ach.GetConditionHitCount(0, 1));

        memory[2] = 22; // first condition is true, sum is correct
        AssertSetTest(ach, true, true, false);
        Assert::AreEqual(0U, ach.GetConditionHitCount(0, 0)); // AddSource condition does not have hit tracking
        Assert::AreEqual(2U, ach.GetConditionHitCount(0, 1));
    }

    TEST_METHOD(TestSubSource)
    {
        unsigned char memory[] = { 0x00, 0x12, 0x34, 0xAB, 0x56 };
        InitializeMemory(memory, 5);

        AchievementHarness ach;
        ach.ParseTrigger("B:0xH0002=0_0xH0001=14"); // NOTE: SubSource subtracts the first value from the second!
        AssertSetTest(ach, false, false, false);
        Assert::AreEqual(0U, ach.GetConditionHitCount(0, 0));
        Assert::AreEqual(0U, ach.GetConditionHitCount(0, 1));

        memory[2] = 4; // difference is correct
        AssertSetTest(ach, true, true, false);
        Assert::AreEqual(0U, ach.GetConditionHitCount(0, 0)); // SubSource condition does not have hit tracking
        Assert::AreEqual(1U, ach.GetConditionHitCount(0, 1));

        memory[1] = 0; // first condition is true, but not difference
        AssertSetTest(ach, false, false, false);
        Assert::AreEqual(0U, ach.GetConditionHitCount(0, 0)); // SubSource condition does not have hit tracking
        Assert::AreEqual(1U, ach.GetConditionHitCount(0, 1));

        memory[2] = 14; // first condition is true, value is negative inverse of expected value
        AssertSetTest(ach, false, false, false);
        Assert::AreEqual(0U, ach.GetConditionHitCount(0, 0)); // SubSource condition does not have hit tracking
        Assert::AreEqual(1U, ach.GetConditionHitCount(0, 1));

        memory[1] = 28; // difference is correct again
        AssertSetTest(ach, true, true, false);
        Assert::AreEqual(0U, ach.GetConditionHitCount(0, 0)); // SubSource condition does not have hit tracking
        Assert::AreEqual(2U, ach.GetConditionHitCount(0, 1));
    }

    TEST_METHOD(TestAddSubSource)
    {
        unsigned char memory[] = { 0x00, 0x12, 0x34, 0xAB, 0x56 };
        InitializeMemory(memory, 5);

        AchievementHarness ach;
        ach.ParseTrigger("A:0xH0001=0_B:0xL0002=0_0xL0004=14"); // byte(1) - low(2) + low(4) == 14
        AssertSetTest(ach, false, false, false);
        Assert::AreEqual(0U, ach.GetConditionHitCount(0, 0));
        Assert::AreEqual(0U, ach.GetConditionHitCount(0, 1));
        Assert::AreEqual(0U, ach.GetConditionHitCount(0, 2));

        memory[1] = 12; // total is correct
        AssertSetTest(ach, true, true, false);
        Assert::AreEqual(0U, ach.GetConditionHitCount(0, 0)); // AddSource condition does not have hit tracking
        Assert::AreEqual(0U, ach.GetConditionHitCount(0, 1)); // SubSource condition does not have hit tracking
        Assert::AreEqual(1U, ach.GetConditionHitCount(0, 2));

        memory[1] = 0; // first condition is true, but not total
        AssertSetTest(ach, false, false, false);
        Assert::AreEqual(0U, ach.GetConditionHitCount(0, 0)); // AddSource condition does not have hit tracking
        Assert::AreEqual(0U, ach.GetConditionHitCount(0, 1)); // SubSource condition does not have hit tracking
        Assert::AreEqual(1U, ach.GetConditionHitCount(0, 2));

        memory[4] = 18; // byte(4) would make total true, but not low(4)
        AssertSetTest(ach, false, false, false);
        Assert::AreEqual(0U, ach.GetConditionHitCount(0, 0)); // AddSource condition does not have hit tracking
        Assert::AreEqual(0U, ach.GetConditionHitCount(0, 1)); // SubSource condition does not have hit tracking
        Assert::AreEqual(1U, ach.GetConditionHitCount(0, 2));

        memory[2] = 1;
        memory[4] = 15; // difference is correct again
        AssertSetTest(ach, true, true, false);
        Assert::AreEqual(0U, ach.GetConditionHitCount(0, 0)); // AddSource condition does not have hit tracking
        Assert::AreEqual(0U, ach.GetConditionHitCount(0, 1)); // SubSource condition does not have hit tracking
        Assert::AreEqual(2U, ach.GetConditionHitCount(0, 2));
    }

    TEST_METHOD(TestAddHits)
    {
        unsigned char memory[] = { 0x00, 0x12, 0x34, 0xAB, 0x56 };
        InitializeMemory(memory, 5);

        AchievementHarness ach;
        ach.ParseTrigger("C:0xH0001=18(2)_0xL0004=6(4)"); // repeated(4, byte(1) == 18 || low(4) == 6)
        AssertSetTest(ach, false, true, false);
        Assert::AreEqual(1U, ach.GetConditionHitCount(0, 0));
        Assert::AreEqual(1U, ach.GetConditionHitCount(0, 1));

        AssertSetTest(ach, true, true, false); // total hits met (2 for each condition)
        Assert::AreEqual(2U, ach.GetConditionHitCount(0, 0));
        Assert::AreEqual(2U, ach.GetConditionHitCount(0, 1));

        AssertSetTest(ach, true, false, false);
        Assert::AreEqual(2U, ach.GetConditionHitCount(0, 0)); // threshold met, stop incrementing
        Assert::AreEqual(2U, ach.GetConditionHitCount(0, 1)); // total met prevents incrementing even though individual tally has not reached total

        ach.Reset();
        AssertSetTest(ach, false, true, false);
        Assert::AreEqual(1U, ach.GetConditionHitCount(0, 0));
        Assert::AreEqual(1U, ach.GetConditionHitCount(0, 1));

        memory[1] = 16;
        AssertSetTest(ach, false, true, false); // 1 + 2 < 4, not met
        Assert::AreEqual(1U, ach.GetConditionHitCount(0, 0));
        Assert::AreEqual(2U, ach.GetConditionHitCount(0, 1));

        AssertSetTest(ach, true, true, false); // 1 + 3 = 4, met
        Assert::AreEqual(1U, ach.GetConditionHitCount(0, 0));
        Assert::AreEqual(3U, ach.GetConditionHitCount(0, 1));
    }

    TEST_METHOD(TestAltGroups)
    {
        unsigned char memory[] = { 0x00, 0x12, 0x34, 0xAB, 0x56 };
        InitializeMemory(memory, 5);

        AchievementHarness ach;
        ach.ParseTrigger("0xH0001=16S0xH0002=52S0xL0004=6");

        // core not true, both alts are
        AssertSetTest(ach, false, true, false);
        Assert::AreEqual(0U, ach.GetConditionHitCount(0, 0));
        Assert::AreEqual(1U, ach.GetConditionHitCount(1, 0));
        Assert::AreEqual(1U, ach.GetConditionHitCount(2, 0));

        memory[1] = 16; // core and both alts true
        AssertSetTest(ach, true, true, false);
        Assert::AreEqual(1U, ach.GetConditionHitCount(0, 0));
        Assert::AreEqual(2U, ach.GetConditionHitCount(1, 0));
        Assert::AreEqual(2U, ach.GetConditionHitCount(2, 0));

        memory[4] = 0; // core and first alt true
        AssertSetTest(ach, true, true, false);
        Assert::AreEqual(2U, ach.GetConditionHitCount(0, 0));
        Assert::AreEqual(3U, ach.GetConditionHitCount(1, 0));
        Assert::AreEqual(2U, ach.GetConditionHitCount(2, 0));

        memory[2] = 0; // core true, but neither alt is
        AssertSetTest(ach, false, true, false);
        Assert::AreEqual(3U, ach.GetConditionHitCount(0, 0));
        Assert::AreEqual(3U, ach.GetConditionHitCount(1, 0));
        Assert::AreEqual(2U, ach.GetConditionHitCount(2, 0));

        memory[4] = 6; // core and second alt true
        AssertSetTest(ach, true, true, false);
        Assert::AreEqual(4U, ach.GetConditionHitCount(0, 0));
        Assert::AreEqual(3U, ach.GetConditionHitCount(1, 0));
        Assert::AreEqual(3U, ach.GetConditionHitCount(2, 0));
    }

    // verifies that a ResetIf resets everything regardless of where it is
    TEST_METHOD(TestResetIfInAltGroup)
    {
        unsigned char memory[] = { 0x00, 0x12, 0x34, 0xAB, 0x56 };
        InitializeMemory(memory, 5);

        AchievementHarness ach;
        ach.ParseTrigger("0xH0001=18(1)_R:0xH0000=1S0xH0002=52(1)S0xL0004=6(1)_R:0xH0000=2");

        AssertSetTest(ach, true, true, false);
        Assert::AreEqual(1U, ach.GetConditionHitCount(0, 0));
        Assert::AreEqual(1U, ach.GetConditionHitCount(1, 0));
        Assert::AreEqual(1U, ach.GetConditionHitCount(2, 0));

        memory[0] = 1; // reset in core group resets everything
        AssertSetTest(ach, false, true, true);
        Assert::AreEqual(0U, ach.GetConditionHitCount(0, 0));
        Assert::AreEqual(0U, ach.GetConditionHitCount(1, 0));
        Assert::AreEqual(0U, ach.GetConditionHitCount(2, 0));

        memory[0] = 0;
        AssertSetTest(ach, true, true, false);
        Assert::AreEqual(1U, ach.GetConditionHitCount(0, 0));
        Assert::AreEqual(1U, ach.GetConditionHitCount(1, 0));
        Assert::AreEqual(1U, ach.GetConditionHitCount(2, 0));

        memory[0] = 2; // reset in alt group resets everything
        AssertSetTest(ach, false, true, true);
        Assert::AreEqual(0U, ach.GetConditionHitCount(0, 0));
        Assert::AreEqual(0U, ach.GetConditionHitCount(1, 0));
        Assert::AreEqual(0U, ach.GetConditionHitCount(2, 0));
    }

    // verifies that PauseIf only pauses the group it's in
    TEST_METHOD(TestPauseIfInAltGroup)
    {
        unsigned char memory[] = { 0x00, 0x12, 0x34, 0xAB, 0x56 };
        InitializeMemory(memory, 5);

        AchievementHarness ach;
        ach.ParseTrigger("0xH0001=18_P:0xH0000=1S0xH0002=52S0xL0004=6_P:0xH0000=2");

        AssertSetTest(ach, true, true, false);
        Assert::AreEqual(1U, ach.GetConditionHitCount(0, 0));
        Assert::AreEqual(1U, ach.GetConditionHitCount(1, 0));
        Assert::AreEqual(1U, ach.GetConditionHitCount(2, 0));

        memory[0] = 1; // pause in core group only pauses core group
        AssertSetTest(ach, false, true, false);
        Assert::AreEqual(1U, ach.GetConditionHitCount(0, 0));
        Assert::AreEqual(2U, ach.GetConditionHitCount(1, 0));
        Assert::AreEqual(2U, ach.GetConditionHitCount(2, 0));

        memory[0] = 0;
        AssertSetTest(ach, true, true, false);
        Assert::AreEqual(2U, ach.GetConditionHitCount(0, 0));
        Assert::AreEqual(3U, ach.GetConditionHitCount(1, 0));
        Assert::AreEqual(3U, ach.GetConditionHitCount(2, 0));

        memory[0] = 2; // pause in alt group only pauses alt group
        AssertSetTest(ach, true, true, false);
        Assert::AreEqual(3U, ach.GetConditionHitCount(0, 0));
        Assert::AreEqual(4U, ach.GetConditionHitCount(1, 0));
        Assert::AreEqual(3U, ach.GetConditionHitCount(2, 0));
    }

    TEST_METHOD(TestPauseIfResetIfAltGroup)
    {
        unsigned char memory[] = { 0x00, 0x12, 0x34, 0xAB, 0x56 };
        InitializeMemory(memory, 5);

        AchievementHarness ach;
        ach.ParseTrigger("0xH0000=0.1._0xH0000=2SP:0xH0001=18_R:0xH0002=52");

        AssertSetTest(ach, false, true, false);
        Assert::AreEqual(1U, ach.GetConditionHitCount(0, 0));

        memory[0] = 1; // move off HitCount
        AssertSetTest(ach, false, true, false);
        Assert::AreEqual(1U, ach.GetConditionHitCount(0, 0));

        memory[1] = 16; // unpause alt group, HitCount should be reset
        AssertSetTest(ach, false, true, true);
        Assert::AreEqual(0U, ach.GetConditionHitCount(0, 0));

        memory[0] = 0;
        memory[1] = 18; // repause alt group, reset hitcount target, hitcount should be set
        AssertSetTest(ach, false, true, false);
        Assert::AreEqual(1U, ach.GetConditionHitCount(0, 0));

        memory[0] = 2; // trigger win condition. alt group has no normal conditions, it should be considered false
        AssertSetTest(ach, false, true, false);
    }
};

} // namespace tests
} // namespace data
} // namespace ra
