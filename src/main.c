#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(current_sensor_modbus, LOG_LEVEL_DBG);

#if !DT_NODE_EXISTS(DT_PATH(zephyr_user)) || \
	!DT_NODE_HAS_PROP(DT_PATH(zephyr_user), io_channels)
#error "No suitable devicetree overlay zephyr_user specified"
#endif
 
void print_adc_cfg(struct adc_channel_cfg* adc_cfg) 
{
	printk("\n");
	printk("adc_channel_cfg\n");
	printk("   .gain %d\n", adc_cfg->gain);
	printk("   .reference %d\n", adc_cfg->reference);
	printk("   .acquisition_time %d\n", adc_cfg->acquisition_time);
	printk("   .channel_id %d\n", adc_cfg->channel_id);
	printk("   .differential %d\n", adc_cfg->differential);
	printk("   .input_positive %d\n", adc_cfg->input_positive);
	printk("   .input_negative %d\n", adc_cfg->input_negative);
	printk("\n");
}

void print_adc_seq(struct adc_sequence* sequence) 
{
	printk("\n");
	printk("adc_sequence\n");
	printk("   .buffer %d\n", sequence->buffer);
	printk("   .buffer_size %d\n", sequence->buffer_size);
	printk("   .calibrate %d\n", sequence->calibrate);
	printk("   .channels %d\n", sequence->channels);
	printk("   .options %d\n", sequence->options);
	printk("   .oversampling %d\n", sequence->oversampling);
	printk("   .resolution %d\n", sequence->resolution);
	printk("\n");
}

void read_mean_sequence_from_adc(const struct adc_dt_spec *adc_spec, 
								 const struct adc_sequence *sequence, 
								 int32_t *result)
{
	int32_t sum = 0;
	int16_t *buffer = sequence->buffer;
	size_t buffer_size = sequence->buffer_size / sizeof(int16_t);
	
	if (adc_read_dt(adc_spec, sequence)) {
		LOG_ERR("ADC [%s] adc_read_dt failed!\n", adc_spec->dev->name);
		return;
	}		
	for(size_t i=0; i<buffer_size; i++) {
		sum += *(buffer+i);
	}
	*result = sum / (int32_t) buffer_size;
}
 

int main(void)
{
	printk("Current Sensors over Modbus TCP on STM32 [%s]\n", CONFIG_BOARD);
	static const struct adc_dt_spec adc_spec  = ADC_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user),0);
	static const struct adc_dt_spec adc_spec2 = ADC_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user),1);
	static const struct adc_dt_spec adc_pa3   = ADC_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user),2);

	if (!adc_is_ready_dt(&adc_spec)) {
		LOG_ERR("ADC [%s] not ready!\n", adc_spec.dev->name);
		return 0;
	}
	if (!adc_is_ready_dt(&adc_spec2)) {
		LOG_ERR("ADC [%s] not ready!\n", adc_spec2.dev->name);
		return 0;
	}	
	if (!adc_is_ready_dt(&adc_pa3)) {
		LOG_ERR("ADC [%s] not ready!\n", adc_pa3.dev->name);
		return 0;
	}

	int ret = 0;
	ret = adc_channel_setup_dt(&adc_pa3);
	if (ret != 0) {
		LOG_ERR("ADC [%s] adc_channel_setup_dt failed with %d!\n", adc_pa3.dev->name, ret);
	}

	static struct adc_channel_cfg adc_cfg2 = 
	{
		.gain             = ADC_GAIN_1,
		.reference        = ADC_REF_INTERNAL,
		// .acquisition_time = ADC_ACQ_TIME(ADC_ACQ_TIME_TICKS, 7),
		.channel_id       = 15,
		// .differential	  = 1,
		.input_positive   = 0,
		// .input_negative   = 1,
	};
	print_adc_cfg(&adc_pa3.channel_cfg);
	print_adc_cfg(&adc_cfg2);
	// ret = adc_channel_setup(adc_pa3.dev, &adc_cfg2);
	// ret = adc_channel_setup_dt(&adc_pa3);
	// if (ret != 0) {
	// 	LOG_ERR("ADC [%s] adc_channel_setup failed with %d!\n", adc_pa3.dev->name, ret);
	// }

	uint16_t m_sample_buffer[6] = {0};
	struct adc_sequence sequenceOO = {
		.buffer = m_sample_buffer,
		.buffer_size = sizeof(m_sample_buffer),
		.resolution = 16,
		.channels = BIT(15),
		// .options = NULL,
		// .calibrate = 0,
	};
	print_adc_seq(&sequenceOO);
	// ret = adc_sequence_init_dt(&adc_pa3, &sequenceOO);
	// if (ret != 0) {
	// 	LOG_ERR("ADC [%s] adc_sequence_init_dt failed with %d!\n", adc_pa3.dev->name, ret);
	// }
	print_adc_seq(&sequenceOO);

	uint32_t tx = k_cycle_get_32();
	ret = adc_read_dt(&adc_pa3, &sequenceOO);
	printk("Took %d µs\n", k_cyc_to_us_floor32(k_cycle_get_32()-tx));
	if (ret != 0) {
		LOG_ERR("ADC [%s] adc_read_dt failed with %d!\n", adc_pa3.dev->name, ret);
	}
	for(size_t i=0; i<ARRAY_SIZE(m_sample_buffer); i++) {
		printk("m_sample_buffer[%d]= %d\n", i, m_sample_buffer[i]);
		k_sleep(K_MSEC(100));
	}
	int32_t result = m_sample_buffer[0];
	ret = adc_raw_to_millivolts_dt(&adc_pa3, &result);
	if (ret != 0) {
		LOG_ERR("ADC [%s] adc_raw_to_millivolts_dt failed with %d!\n", adc_pa3.dev->name, ret);
	}
	printk("result: %d mv\n", result);

	static struct adc_channel_cfg adc_cfg = 
	{
		.gain             = ADC_GAIN_1,
		.reference        = ADC_REF_INTERNAL,
		.acquisition_time = ADC_ACQ_TIME(ADC_ACQ_TIME_TICKS, 7),
		.channel_id       = 0,
		.differential	  = 1,
		.input_positive   = 0,
		.input_negative   = 1,
	};
	adc_channel_setup(adc_spec.dev, &adc_cfg);
	adc_channel_setup(adc_spec2.dev, &adc_cfg);
	// adc_channel_setup_dt(&adc_spec);
	print_adc_cfg(&adc_cfg);
	print_adc_cfg(&adc_spec.channel_cfg);

	// struct adc_sequence_options opts = 
	// {
    //     .interval_us = 1e6,
    //     .extra_samplings = 0,
	// };
	int16_t buf[16] = {0};
	struct adc_sequence sequence = {
		.options = NULL,
		.buffer = buf,
		.buffer_size = sizeof(buf),
		.resolution = 16,
		.channels = BIT(0),
		.oversampling = 0,
		.calibrate = 0
	};

	// int o = adc_read_async(adc_spec.dev, &sequence, NULL);
	// printk("%d\n", o);

	#define OFFSET_ZERO_A 27

	while(1) {
		uint32_t t0 = k_cycle_get_32();
		int32_t result = 0;
		read_mean_sequence_from_adc(&adc_spec, &sequence, &result);

		printk("Took %d ms\n", k_cyc_to_ms_floor32(k_cycle_get_32()-t0));
		printk("ADC raw (int32_t) %d dig\n", result);
		result += OFFSET_ZERO_A;
		printk("ADC +offset (int32_t) %d dig\n", result);

		if (adc_raw_to_millivolts_dt(&adc_spec, &result)) {
			LOG_ERR("ADC [%s] can't convert to mv!\n", adc_spec.dev->name);
			return 0;
		}
		printk("ADC reading (int32_t) %d mv\n", result);
		float current = (result)  * 0.1827821;
		printk("ADC reading (int32_t) %.2f A\n", current);
		printk("\n");


		t0 = k_cycle_get_32();
		result = 0;
		read_mean_sequence_from_adc(&adc_spec2, &sequence, &result);

		printk("Took %d ms\n", k_cyc_to_ms_floor32(k_cycle_get_32()-t0));
		printk("ADC2 raw (int32_t) %d dig\n", result);
		result += OFFSET_ZERO_A;
		printk("ADC2 +offset (int32_t) %d dig\n", result);

		if (adc_raw_to_millivolts_dt(&adc_spec2, &result)) {
			LOG_ERR("ADC2 [%s] can't convert to mv!\n", adc_spec2.dev->name);
			return 0;
		}
		printk("ADC2 reading (int32_t) %d mv\n", result);
		current = (result)  * 0.1827821;
		printk("ADC2 reading (int32_t) %.2f A\n", current);
		printk("\n");
		
		
		k_sleep(K_MSEC(5000));
	}

	return 0;
}