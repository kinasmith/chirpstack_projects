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
  data.HEA_TEM = ((input.bytes[36] << 8) + input.bytes[37])/100 - 50;
  data.REF_VOL = ((input.bytes[38] << 8) + input.bytes[39])/100;
  data.BAT_VOL = ((input.bytes[40] << 8) + input.bytes[41])/100;
  return {
    data: data //,
    // warnings: warnings
  };
}



// Decode decodes an array of bytes into an object.
//  - fPort contains the LoRaWAN fPort number
//  - bytes is an array of bytes, e.g. [225, 230, 255, 0]
//  - variables contains the device variables e.g. {"calibration": "3.5"} (both the key / value are of type string)
// The function must return an object, e.g. {"temperature": 22.5}
// 0, 106.00   DIR_MIN
// 1, 278.00   DIR_AVG
// 2, 27.00    DIR_MAX
// 3, 0.10     SPD_LUL
// 4, 0.10     SPD_AVG
// 5, 0.20     SPD_GST
// 6, 19.70    AIR_TEM
// 7, 19.90    AIR_I-T
// 8, 32.00    AIR_HUM
// 9, 837.30   AIR_PRS
// 10, 0.00    RAI_AMT
// 11, 0.00    RAI_DUR
// 12, 0.00    RAI_INT
// 13, 0.00    HAI_AMT
// 14, 0.00    HAI_DUR
// 15, 0.00    HAI_INT
// 16, 0.00    RAI_PEK
// 17, 0.00    HAI_PEK
// 18, 19.00   HEA_TEM
// 19, 3.54    REF_VOL
// 20, x.xx    BAT_VOL


function Decode(fPort, bytes, variables) {
  var decoded = {
    "DIR_MIN":{},
    "DIR_AVG":{},
    "DIR_MAX":{},
    "SPD_LUL":{},
    "SPD_AVG":{},
    "SPD_GST":{},
    "AIR_TEM":{},
    "AIR_INT":{},
    "AIR_HUM":{},
    "AIR_PRS":{},
    "RAI_AMT":{},
    "RAI_DUR":{},
    "RAI_INT":{},
    "HAI_AMT":{},
    "HAI_DUR":{},
    "HAI_INT":{},
    "RAI_PEK":{},
    "HAI_PEK":{},
    "HEA_TEM":{},
    "REF_VOL":{},
    "BAT_VOL":{} 
  };
  decoded.DIR_MIN = ((bytes[0] << 8) + bytes[1])/100;
  decoded.DIR_AVG = ((bytes[2] << 8) + bytes[3])/100;
  decoded.DIR_MAX = ((bytes[4] << 8) + bytes[5])/100;
  decoded.SPD_LUL = ((bytes[6] << 8) + bytes[7])/100;
  decoded.SPD_AVG = ((bytes[8] << 8) + bytes[9])/100;
  decoded.SPD_GST = ((bytes[10] << 8) + bytes[11])/100;
  decoded.AIR_TEM = ((bytes[12] << 8) + bytes[13])/100 - 50;
  decoded.AIR_INT = ((bytes[14] << 8) + bytes[15])/100 - 50;
  decoded.AIR_HUM = ((bytes[16] << 8) + bytes[17])/100;
  decoded.AIR_PRS = ((bytes[18] << 8) + bytes[19])/10;
  decoded.RAI_AMT = ((bytes[20] << 8) + bytes[21])/100;
  decoded.RAI_DUR = ((bytes[22] << 8) + bytes[23])/100;
  decoded.RAI_INT = ((bytes[24] << 8) + bytes[25])/100;
  decoded.HAI_AMT = ((bytes[26] << 8) + bytes[27])/100;
  decoded.HAI_DUR = ((bytes[28] << 8) + bytes[29])/100;
  decoded.HAI_INT = ((bytes[30] << 8) + bytes[31])/100;
  decoded.RAI_PEK = ((bytes[32] << 8) + bytes[33])/100;
  decoded.HAI_PEK = ((bytes[34] << 8) + bytes[35])/100;
  decoded.HEA_TEM = ((bytes[36] << 8) + bytes[37])/100 - 50;
  decoded.REF_VOL = ((bytes[38] << 8) + bytes[39])/100;
  decoded.BAT_VOL = ((bytes[40] << 8) + bytes[41])/100;
  return decoded;
}