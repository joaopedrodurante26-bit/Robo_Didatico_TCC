#include <Arduino.h>
#include <unity.h>

#include "motores/motores.h"

void setUp(void) {
    initMotores();
}

void tearDown(void) {}

void test_motores_avanco_e_re(void) {
    moverFrente(120);
    delay(1000);

    moverTras(120);
    delay(1000);

    pararMotores();

    TEST_ASSERT_TRUE(true);
}

void setup() {
    delay(1000);
    UNITY_BEGIN();
    RUN_TEST(test_motores_avanco_e_re);
    UNITY_END();
}

void loop() {}
