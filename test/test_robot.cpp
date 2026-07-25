#include <unity.h>
#include "robot/robot.h"

void setUp(void) {}
void tearDown(void) {}

void test_defaults_to_idle(void) {
    initRobot();
    TEST_ASSERT_EQUAL(MODE_IDLE, getCurrentMode());
}

void test_mode_changes_are_registered(void) {
    initRobot();
    setRobotMode(MODE_REMOTE);
    TEST_ASSERT_EQUAL(MODE_REMOTE, getCurrentMode());
    TEST_ASSERT_EQUAL_STRING("MODE_REMOTE", robotModeToString(getCurrentMode()));
}

void test_test_mode_is_supported(void) {
    initRobot();
    setRobotMode(MODE_TEST);
    TEST_ASSERT_EQUAL(MODE_TEST, getCurrentMode());
}

int main(int argc, char** argv) {
    UNITY_BEGIN();
    RUN_TEST(test_defaults_to_idle);
    RUN_TEST(test_mode_changes_are_registered);
    RUN_TEST(test_test_mode_is_supported);
    return UNITY_END();
}
