

#ifndef WEB_CONFIG_H
#define WEB_CONFIG_H

void handleRoot() {
  configServer.send(200, "text/html", "<form method='post' action='/submit'>\
    <label for='server'>Server:</label><input type='text' id='server' name='server'><br>\
    <label for='port'>Port:</label><input type='text' id='port' name='port'><br>\
    <label for='user'>User:</label><input type='text' id='user' name='user'><br>\
    <label for='password'>Password:</label><input type='password' id='password' name='password'><br>\
    <input type='submit' value='Submit'>\
    </form>");
}

boolean isValidNumber(String str){
  for(byte i=0;i<str.length();i++)
  {
  if(isDigit(str.charAt(i))) return true;
  }
  return false;
}

void handleSubmit() {
  String serverValue = configServer.arg("server");
  String portValue = configServer.arg("port");
  String userValue = configServer.arg("user");
  String passwordValue = configServer.arg("password");
  
  mqttServer = serverValue;
  mqttPort = portValue.toInt();
  mqttUser = userValue;
  mqttPassword = passwordValue;


  if(isValidNumber(portValue)){
    buggyBridgeState = END_CONFIG;
    configServer.send(200, "text/plain", "Thank you for configuration, lets try MQTT connection!");
  }else{
    configServer.send(400, "text/plain", "Port should be an integer number!");    
    }
}

#endif
