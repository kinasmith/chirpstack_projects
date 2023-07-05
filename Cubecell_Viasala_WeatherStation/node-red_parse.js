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
    SUP_VOL: data.SUP_VOL,
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


/*
function decodeUplink(input) {
  var data = {};
  data.DIR_MIN = ((input.bytes[0] << 8) + input.bytes[1])/100;
  data.DIR_AVG = ((input.bytes[2] << 8) + input.bytes[3])/100;
  data.DIR_MAX = ((input.bytes[4] << 8) + input.bytes[5])/100;
  data.SPD_LUL = ((input.bytes[6] << 8) + input.bytes[7])/100;
  data.SPD_AVG = ((input.bytes[8] << 8) + input.bytes[9])/100;
  data.SPD_GST = ((input.bytes[10] << 8) + input.bytes[11])/100;
  data.AIR_TEM = ((input.bytes[12] << 8) + input.bytes[13])/100 - 50;
  data.AIR_INT = ((input.bytes[14] << 8) + input.bytes[15])/100 - 50;
  data.AIR_HUM = ((input.bytes[16] << 8) + input.bytes[17])/100;
  data.AIR_PRS = ((input.bytes[18] << 8) + input.bytes[19])/10;
  data.RAI_AMT = ((input.bytes[20] << 8) + input.bytes[21])/100;
  data.RAI_DUR = ((input.bytes[22] << 8) + input.bytes[23])/100;
  data.RAI_INT = ((input.bytes[24] << 8) + input.bytes[25])/100;
  data.HAI_AMT = ((input.bytes[26] << 8) + input.bytes[27])/100;
  data.HAI_DUR = ((input.bytes[28] << 8) + input.bytes[29])/100;
  data.HAI_INT = ((input.bytes[30] << 8) + input.bytes[31])/100;
  data.RAI_PEK = ((input.bytes[32] << 8) + input.bytes[33])/100;
  data.HAI_PEK = ((input.bytes[34] << 8) + input.bytes[35])/100;
  data.SUP_VOL = ((input.bytes[36] << 8) + input.bytes[37])/100 - 50;
  data.REF_VOL = ((input.bytes[38] << 8) + input.bytes[39])/100;
  data.BAT_VOL = ((input.bytes[40] << 8) + input.bytes[41])/100;
  return {
    data: data //,
    // warnings: warnings
  };
}
*/