# APC_UPS_controller
Monitor an APC UPS over WiFi

### Features
- Based on a Node MCU.
- Compatible with the [APC Smart UPS protocol](https://kirbah.github.io/apc-ups/Smart-protocol/)
- Display main parameters of the UPS on the LCD screen.
- Send data to the remote server via HTTPS ( configurable ).
- Shutdown the UPS when a battery is discharged to a given threshold ( configurable ).
- Command line interface for the configuring.
- An example of the remote server is included.

### Command line
To enter Commadline mode, you need to press the button after booting, while on the LCD screen the message 'Wait for press...' appears.
Communication parameters for terminal: 115200,8N1.

| Command *arg* | Explanation |
| --- | --- |
| delay *number* | Set delay for waiting button press during boot (1-60, seconds) |
| poweroff *number* | Set battery threshold for power off (0-99, percents, 0=disable) |
| standalone *digit* | Set standalone mode (0/1, 1=standlone). In this mode network disabled |
| name *word* | Set UPS name, used as a device identifier on the remote server |
| ssid *word* | Set WiFi SSID |
| passw *word* | Set WiFi password |
| host *word* | Set destination host ( hostname or IPv4 ) |
| port *number* | Set destination port |
| uri *word* | Set destination URI ( like "/cgi-bin/ups.cgi" ) |
| auth *word* | Set HTTP(S) authorization (0/1, 1=enable) |
| huser *word* | Set HTTP(S) username |
| hpassw *word* | Set HTTP(S) passwor d|
| show | Show current configuration |
| save | Save configuration to EEPROM |
| reboot [ *hard* or *soft* ] | Reboot, *hard* doing ESP.reset(), *soft* doing ESP.restart(), default is *soft* |
| help | Get help |

### Transmitting data to the remote side
Data are transmitted in the POST request.

| Field | Value |
| --- | --- |
| uptime | Uptime in form "1d1h01m1s", not very accurate |
| name | Name of the device which set via the command line |
| model | UPS model which UPS reported |
| boot | always "1", this is 1st  message after boot |
| alarm | Emergency messages like a main power failure |
| msg | Infomational messages |
| data | Comma separated lists of main parameters: battery voltage (float), temperature (float), line voltage (int), power load (float), battery level (int), UPS status (hex) |


