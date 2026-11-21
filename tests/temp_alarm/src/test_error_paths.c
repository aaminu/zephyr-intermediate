/*
 * temp_alarm - Error-path unit tests (homework skeleton)
 *
 * Task 2: fill in the stubs according to TEST_SPEC.md. Each stub calls
 * ztest_test_skip(), so the binary builds and runs cleanly until the test is
 * implemented.
 *
 * Run:
 *   west twister -T tests/temp_alarm -p native_sim
 */

#include <errno.h>
#include <zephyr/fff.h>
#include <zephyr/ztest.h>

#include "sensor_fake.h"
#include "temp_alarm.h"

#define DEFAULT_THRESHOLD 30

/* Same before() hook as in test_basic.c: a clean fake and an initialised module. */
static void before(void *fixture)
{
	ARG_UNUSED(fixture);

	sensor_fake_reset();
	sensor_fake_init();

	zassume_ok(temp_alarm_init(&fake_sensor_dev, DEFAULT_THRESHOLD),
		   "precondition: temp_alarm_init must succeed");
}

/*
 * ============================================================================
 * Test Suite: temp_alarm_sensor_errors
 *
 * Failures from the sensor driver, injected through the fakes.
 * ============================================================================
 */
ZTEST_SUITE(temp_alarm_sensor_errors, NULL, NULL, before, NULL, NULL);

ZTEST(temp_alarm_sensor_errors, test_fetch_error_propagates)
{
	/* TODO(l8-task2): a failed fetch is reported,
	 * and the module does not go on to read the channel.
	 * See TEST_SPEC.md "Suite temp_alarm_sensor_errors" #1.
	 */
	struct temp_alarm_status status;

    /* Simulate a sensor fetch failure */
    fake_sensor_sample_fetch_fake.return_val = -EIO;

    int ret = temp_alarm_check();

    zassert_equal(ret, -EIO, "Expected -EIO from failed fetch");
    zassert_ok(temp_alarm_get_status(&status), "Failed to get status");
    zassert_equal(status.last_error, -EIO, "Last error should be -EIO");
    zassert_equal(fake_sensor_sample_fetch_fake.call_count, 1, "Sample fetch must be called once");
    zassert_equal(fake_sensor_channel_get_fake.call_count, 0, "Channel get must not run after fetch failure");
}

ZTEST(temp_alarm_sensor_errors, test_channel_get_error_propagates)
{
	/* TODO(l8-task2): a failed channel read is reported.
	 * See TEST_SPEC.md "Suite temp_alarm_sensor_errors" #2.
	 */
	struct temp_alarm_status status;

    /* Override the channel_get custom fake */
    fake_sensor_channel_get_fake.custom_fake = NULL;
    fake_sensor_channel_get_fake.return_val = -EIO;

    int ret = temp_alarm_check();

    zassert_equal(ret, -EIO, "Expected -EIO from channel_get");
    zassert_ok(temp_alarm_get_status(&status), "Failed to get status");
    zassert_equal(status.last_error, -EIO, "Last error should be -EIO");
    zassert_equal(fake_sensor_sample_fetch_fake.call_count, 1, "Sample fetch must be called once");
    zassert_equal(fake_sensor_channel_get_fake.call_count, 1, "Channel get must be called once");
}

ZTEST(temp_alarm_sensor_errors, test_fail_then_recover)
{
	/* TODO(l8-task2): the first fetch fails and the second one succeeds.
	 * The module must recover on the second check.
	 * See TEST_SPEC.md "Suite temp_alarm_sensor_errors" #3.
	 */
	struct temp_alarm_status status;

    /* First fetch fails; second succeeds */
    static int fetch_results[] = { -EIO, 0 };

    SET_RETURN_SEQ(fake_sensor_sample_fetch,  fetch_results,  ARRAY_SIZE(fetch_results));
    sensor_fake_set_temperature(20);

    /* First attempt: failure */
    zassert_equal(temp_alarm_check(), -EIO, "First fetch must fail");

    /* Second attempt: recovery */
    zassert_ok(temp_alarm_check(), "Second fetch must succeed");
    zassert_ok(temp_alarm_get_status(&status), "Failed to get status");
    zassert_equal(status.last_temp, 20, "Recovered reading must be 20 C");
    zassert_false(status.is_alarming, "Alarm should remain inactive");
    zassert_equal(status.last_error, -EIO, "Last error must be preserved");
    zassert_equal(fake_sensor_sample_fetch_fake.call_count, 2, "Expected two fetch attempts");
    zassert_equal(fake_sensor_channel_get_fake.call_count, 1, "Channel get must only run after successful fetch");
}

/*
 * ============================================================================
 * Test Suite: temp_alarm_invalid_input
 *
 * Arguments the public API must reject.
 * ============================================================================
 */
ZTEST_SUITE(temp_alarm_invalid_input, NULL, NULL, before, NULL, NULL);

ZTEST(temp_alarm_invalid_input, test_get_status_null)
{
	/* TODO(l8-task2): a NULL status pointer is rejected.
	 * See TEST_SPEC.md "Suite temp_alarm_invalid_input" #1.
	 */
	int ret = temp_alarm_get_status(NULL);

    zassert_equal(ret, -EINVAL, "NULL status must return -EINVAL");
}

ZTEST(temp_alarm_invalid_input, test_init_without_device)
{
	/* TODO(l8-task2): init without a sensor device is rejected.
	 * See TEST_SPEC.md "Suite temp_alarm_invalid_input" #2.
	 */
	int ret = temp_alarm_init(NULL, DEFAULT_THRESHOLD);

    zassert_equal(ret, -ENODEV, "NULL device must return -ENODEV");
}

ZTEST(temp_alarm_invalid_input, test_init_threshold_out_of_range)
{
	/* TODO(l8-task2): init rejects thresholds outside the allowed range.
	 * See TEST_SPEC.md "Suite temp_alarm_invalid_input" #3.
	 */
	int ret;

    /* Below minimum threshold */
    ret = temp_alarm_init(&fake_sensor_dev, -1);

    zassert_equal(ret, -EINVAL, "Negative threshold must be rejected");

    /* Above maximum threshold */
    ret = temp_alarm_init(&fake_sensor_dev, 101);

    zassert_equal(ret, -EINVAL, "Threshold above 100 must be rejected");
}

ZTEST(temp_alarm_invalid_input, test_set_threshold_out_of_range)
{
	/* TODO(l8-task2): set_threshold rejects values outside the allowed range,
	 * and keeps the old threshold.
	 * See TEST_SPEC.md "Suite temp_alarm_invalid_input" #4.
	 */
	struct temp_alarm_status status;

    /* Both invalid updates must fail */
    zassert_equal(temp_alarm_set_threshold(-1), -EINVAL, "Negative threshold must be rejected");
    zassert_equal(temp_alarm_set_threshold(101), -EINVAL, "Threshold above 100 must be rejected");

    /*
     * The original threshold must still be 30 C and a reading of 40 C must therefore trigger an alarm.
     */
    sensor_fake_set_temperature(40);

    zassert_ok(temp_alarm_check(), "Temperature check should succeed");
    zassert_ok(temp_alarm_get_status(&status),"Failed to get status");
    zassert_true(status.is_alarming, "Original threshold of 30 C must remain active");
    zassert_equal(status.alarm_count, 1, "Expected one alarm event");
}
