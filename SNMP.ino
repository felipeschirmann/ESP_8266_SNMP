void setupVarCalbacksSNMP()
{
    // Get callbacks from creating a handler for each of the OID
  callbackIfSpeed = snmp.addGuageHandler(router, oidIfSpeedGuage, &ifSpeedResponse);
  
  callbackInOctets= snmp.addCounter32Handler(router, oidInOctetsCount32, &inOctetsResponse);
  callbackOutOctets= snmp.addCounter32Handler(router, oidOutOctetsCount32, &outOctetsResponse);
  
  callbackServices = snmp.addIntegerHandler(router, oidServiceCountInt, &servicesResponse);
  callbackSysName = snmp.addStringHandler(router, oidSysName, &sysNameResponse);
//  callback64Counter = snmp.addCounter64Handler(router, oid64Counter, &hcCounter);
  callbackUptime = snmp.addTimestampHandler(router, oidUptime, &uptime);

}

void doSNMPCalculations()
{

  if (uptime == lastUptime)
  {
    Serial.println("Data not updated between polls");
    return;
  }
  else if (uptime < lastUptime)
  { // Check if device has rebooted which will reset counters
    Serial.println("Uptime < lastUptime. Device restarted?");
  }
  else
  {
    if (inOctetsResponse > 0 && ifSpeedResponse > 0 && lastInOctets > 0)
    {
      if (inOctetsResponse > lastInOctets)
      {
        bandwidthInUtilPct = ((float)((inOctetsResponse - lastInOctets) * 8) / (float)(ifSpeedResponse * ((uptime - lastUptime) / 100)) * 100);
        velocitIn = ((float)((inOctetsResponse - lastInOctets) * 8) / (float)(ifSpeedResponse * (uptime - lastUptime)) * 100);
      }
      else if (lastInOctets > inOctetsResponse)
      {
        Serial.println("inOctets Counter wrapped");
        bandwidthInUtilPct = (((float)((4294967295 - lastInOctets) + inOctetsResponse) * 8) / (float)(ifSpeedResponse * ((uptime - lastUptime) / 100)) * 100);
      }
    }
    
    if (outOctetsResponse > 0 && ifSpeedResponse > 0 && lastOutOctets > 0)
    {
      if (outOctetsResponse > lastOutOctets)
      {
        bandwidthOutUtilPct = ((float)((outOctetsResponse - lastOutOctets) * 8) / (float)(ifSpeedResponse * ((uptime - lastUptime) / 100)) * 100);
        velocitOut = ((float)((outOctetsResponse - lastOutOctets) * 8) / (float)(ifSpeedResponse * (uptime - lastUptime)) * 100);
      }
      else if (lastOutOctets > outOctetsResponse)
      {
        Serial.println("outOctets Counter wrapped");
        bandwidthOutUtilPct = (((float)((4294967295 - lastOutOctets) + outOctetsResponse) * 8) / (float)(ifSpeedResponse * ((uptime - lastUptime) / 100)) * 100);
      }
    }
  }
  // Update last samples
  lastUptime = uptime;
  lastInOctets = inOctetsResponse;
  lastOutOctets = outOctetsResponse;
}

void getSNMP()
{
  // Build a SNMP get-request add each OID to the request
  snmpRequest.addOIDPointer(callbackIfSpeed);
  snmpRequest.addOIDPointer(callbackInOctets);
  snmpRequest.addOIDPointer(callbackOutOctets);
  snmpRequest.addOIDPointer(callbackServices);
  snmpRequest.addOIDPointer(callbackSysName);
//  snmpRequest.addOIDPointer(callback64Counter);
  snmpRequest.addOIDPointer(callbackUptime);

  snmpRequest.setIP(WiFi.localIP()); // IP of the listening MCU
  // snmpRequest.setPort(501);  // Default is UDP port 161 for SNMP. But can be overriden if necessary.
  snmpRequest.setUDP(&udp);
  snmpRequest.setRequestID(rand() % 5555);
  snmpRequest.sendTo(router);
  snmpRequest.clearOIDList();
}

void printVariableValues()
{
  if ( DEBUG == 1 ) {
    Serial.printf("ifSpeedResponse: %d\n", ifSpeedResponse);
    Serial.printf("Bandwidth In Utilisation %%: %.1f\n", bandwidthInUtilPct);
    Serial.printf("Bandwidth Out Utilisation %%: %.1f\n", bandwidthOutUtilPct);
    Serial.printf("lastOutOctets: %d\n", lastOutOctets);
    Serial.printf("lastInOctets: %d\n", lastInOctets);
    Serial.printf("inOctetsResponse: %d\n", inOctetsResponse);
    Serial.printf("outOctetsResponse: %d\n", outOctetsResponse);
    Serial.printf("servicesResponse: %d\n", servicesResponse);
    Serial.printf("sysNameResponse: %s\n", sysNameResponse);
    Serial.printf("Uptime: %d\n", uptime);
//    Serial.printf("HCCounter: %llu\n", hcCounter);
  }

}
