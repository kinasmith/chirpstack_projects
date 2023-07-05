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
  var warnings = [];
  if (data.BAT_VOL < 3.6) {
    warnings.push("LOW BATTERY");
  }
  return {
    data: data,
    warnings: warnings
  };
}

function normalizeUplink(input) {
  return {
    data: {
      air: {
        temperature: input.data.AIR_TEM
        relativeHumidity: input.data.AIR_HUM
        pressure: input.data.AIR_PRS
      },
      wind: {
        speed: input.data.SPD_AVG
        direction: input.data.DIR_AVG
      }
    },
    warnings: input.warnings
  }
}
