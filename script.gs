function doGet(e) {
  Logger.log("--- doGet ---");


  if (e != null) {

    value = e.parameters.value;

    // save the data to spreadsheet
    save_data(value);


    return ContentService.createTextOutput("Wrote:" + "\n  value: " + value);
  }
  else {
    Logger.log("here");
    return ContentService.createTextOutput("oops....");
  }
}


function save_data(value) {
  Logger.log("--- save_data ---");


  try {
    var dateTime = new Date();

    var ss = SpreadsheetApp.openByUrl("https://docs.google.com/spreadsheets/d/11EmPq0yy1JHLZguGBsY__woSPoI0ajVRcypYtP0Voe0/edit");
    var dataLoggerSheet = ss.getSheetByName("itemsCount");

    // Get last edited row from DataLogger sheet
    var row = dataLoggerSheet.getLastRow() + 1;


    // Start Populating the data
    dataLoggerSheet.getRange("A" + row).setValue(dateTime.toLocaleDateString() ); // dateTime
    dataLoggerSheet.getRange("B" + row).setValue(dateTime.toLocaleTimeString()); // dateTime
    dataLoggerSheet.getRange("C" + row).setValue(value); // value
  }

  catch (error) {
    Logger.log(JSON.stringify(error));
  }

  Logger.log("--- save_data end---");
}