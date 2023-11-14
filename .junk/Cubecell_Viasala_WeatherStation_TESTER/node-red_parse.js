var data = msg.payload.uplink_message.decoded_payload;
var metadata = msg.payload.uplink_message.rx_metadata[0];
var settings = msg.payload.uplink_message.settings;
var ver_ids = msg.payload.uplink_message.version_ids;
var dev_ids = msg.payload.end_device_ids;
var output;


output = [{ //First object is named fields
    DIR_MIN: data.DIR_MIN,
    DIR_AVG: data.DIR_AVG,
    DIR_MAX: data.DIR_MAX,
    SPD_LUL: data.SPD_LUL,
    SPD_AVG: data.SPD_AVG,
    SPD_GST: data.SPD_GST,
    AIR_TEM: data.AIR_TEM,
    AIR_INT: data.AIR_INT,
    AIR_HUM: data.AIR_HUM,
    AIR_PRS: data.AIR_PRS,
    RAI_AMT: data.RAI_AMT,
    RAI_DUR: data.RAI_DUR,
    RAI_INT: data.RAI_INT,
    HAI_AMT: data.HAI_AMT,
    HAI_DUR: data.HAI_DUR,
    HAI_INT: data.HAI_INT,
    RAI_PEK: data.RAI_PEK,
    HAI_PEK: data.HAI_PEK,
    HEA_TEM: data.HEA_TEM,
    REF_VOL: data.REF_VOL,
    BAT_VOL: data.BAT_VOL,


    fPort: msg.payload.uplink_message.f_port,
    fCnt: msg.payload.uplink_message.f_cnt,
    
    tx_frequency: parseInt(settings.frequency),
    bandwidth: settings.data_rate.lora.bandwidth,
    spreading_factor: settings.data_rate.lora.spreading_factor,
    rssi: metadata.rssi,
    snr: metadata.snr
    },
    { //Second value is tags
        dev_brand: ver_ids.brand_id,
        dev_model: ver_ids.model_id,
        applicationName: dev_ids.application_ids.application_id,
        devEUI: dev_ids.dev_eui
}];

var newMsg = { 
    measurement: dev_ids.application_ids.application_id,
    payload: output 
};
return newMsg;
