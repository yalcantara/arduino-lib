#include <Arduino.h>
#include <unity.h>

void test_dcsensor_uses_overridden_reading();
void test_dcsensor_calibration();
void test_frog_relays_setup_forces_both_relays_off();
void test_frog_relays_use_break_before_make();
void test_pad_with_spaces_handles_any_value_width();
void test_pcf_setup_sets_all_pins_high();
void test_pcf_reads_a_byte_and_reports_no_data_as_all_high();
void test_pcf_changes_one_output_bit();
void test_rail_crossing_turns_off_at_the_configured_time();
void test_rail_crossing_timer_survives_millis_rollover();
void test_sma_averages_only_available_recent_values();
void test_timer_fires_only_after_its_timeout();
void test_turnout_setup_establishes_state_for_each_type();
void test_turnout_moves_slowly_and_waits_for_servo_to_settle();
void test_turnout_switching_operations_select_expected_routes();

void setUp() {}

void tearDown() {}

void setup()
{
  delay(2000);

  UNITY_BEGIN();
  RUN_TEST(test_pad_with_spaces_handles_any_value_width);
  RUN_TEST(test_sma_averages_only_available_recent_values);
  RUN_TEST(test_timer_fires_only_after_its_timeout);
  RUN_TEST(test_turnout_setup_establishes_state_for_each_type);
  RUN_TEST(test_turnout_moves_slowly_and_waits_for_servo_to_settle);
  RUN_TEST(test_turnout_switching_operations_select_expected_routes);
  RUN_TEST(test_pcf_setup_sets_all_pins_high);
  RUN_TEST(test_pcf_reads_a_byte_and_reports_no_data_as_all_high);
  RUN_TEST(test_pcf_changes_one_output_bit);
  RUN_TEST(test_rail_crossing_turns_off_at_the_configured_time);
  RUN_TEST(test_rail_crossing_timer_survives_millis_rollover);
  RUN_TEST(test_frog_relays_setup_forces_both_relays_off);
  RUN_TEST(test_frog_relays_use_break_before_make);
  RUN_TEST(test_dcsensor_uses_overridden_reading);
  RUN_TEST(test_dcsensor_calibration);
  UNITY_END();
}

void loop() {}
