#ifndef GLOBAL_H
#define GLOBAL_H

#define FIRMWARE 	"v3.23 2017-05-13"
String fwVers = "v9.99";

#include "password.h"

IPAddress ip9    	(192, 168, 179, 200);
IPAddress esp_ip   	(0, 0, 0, 0);
IPAddress dns   	(192, 168, 179, 1  );
IPAddress gateway 	(192, 168, 179, 1  );
IPAddress subnet  	(255, 255, 255, 0  );
IPAddress ip0   	(0, 0, 0, 0);
IPAddress dns0    	(0, 0, 0, 0);
IPAddress gateway0  (0, 0, 0, 0);
String esp_ip_str = "noipyet";

boolean 	SoftAPup 		= false;	// Enable Admin Mode for a given Time

/* Debugging */
#define DEBUGGING(...) Serial.println( __VA_ARGS__ )
#define DEBUGGING_L(...) Serial.print( __VA_ARGS__ )


#endif
