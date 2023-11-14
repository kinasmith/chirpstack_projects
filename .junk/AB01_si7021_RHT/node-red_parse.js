var data = msg.payload.uplink_message.decoded_payload;
var metadata = msg.payload.uplink_message.rx_metadata[0];
var settings = msg.payload.uplink_message.settings;
var ver_ids = msg.payload.uplink_message.version_ids;
var dev_ids = msg.payload.end_device_ids;
var output;


output = [{ //First object is named fields
    battery: data.battery,
    humidity: data.humidity,
    temperature: data.temperature,
    
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
