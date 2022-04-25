# smartserver-driver-template
Example custom driver template for the Dialog SmartServer IoT Edge Server (Dialog Semiconductor is a Renesas Electronics company). Documentation is available at http://iecdocs.renesas.com.

Create and test a custom driver for the SmartServer IoT by following these steps:
1.  Clone the IDL example at this URL: https://github.com/izot/smartserver-driver-template.git into your 
    SmartServer IoT.
2.  Using an editor open the Makefile and change the driver to a driver-specific information :
	The driver identifier, driver name, description, manufacturer, license, version, filetype, an extension.
3.  Add any required additional components such as a protocol stack to the build script.
4.  If necessary, change/add/complete the implementation of the required call-back functions.  The example driver
    provides a minimum implementation required to do device creation, provisioning, reading, writing and deletion.
5.  Run make to build the new driver.  
	The output of the build process will be a GLPO file with an installation script and driver image embedded in the file.  The default output will be in build/release/example_driver.glpo.  Copy this glpo file along with the 
	XIF files to your workstation running the SmartServer IoT CMS software.
8.  Create a new driver-specific XIF file by copying and modifying the example_xif.xpl file found in the 
    smartserver-driver-template project.
9.  Using the SmartServer IoT CMS' Device Widget with the SEGMENT CONTROLLER tab selected: 
    * Click on the vertical three dots next to the SmartServer IoT to select the Update menu.
    * Drag and drop the newly created GLPO file into the Update Loader (Drop new update loader here) box, 
    * Click the file icon to save the driver package into the CMS and click the import icon to load the driver to the SmartServer IoT.  Some time later, the new driver will be loaded and running as service (cdriver:<your driver identifier>) under Supervisorctl.
10. Using the CMS' Device Type Widget: 
	  * Click on the Import device type icon and then drag and drop the example_xif.xpl (<driver identifier>_xif.ext) file into the "DROP FILE HERE" box and click on the IMPORT FILE button to load the XIF file to the SmartServer IoT.
    * Click on the BACK button and wait until the device type shows up in the Device Type Widget.
11. Using the SmartServer IoT CMS' Device Widget with the EDGE DEVICES tab selected:
	  * Click on the "+" button to create a device with the following info:
		  name: <your device name>_1,  UID: 001, Integration Method: Manual assignment, Driver: <your driver identifier>, and select the one and only available device types and click the SAVE button.
	    Wait until the newly created device shows up in the Device Widget and shown in blue (not purple color).
    * Click on the three vertical dots next to the newly created device and select Provision menu.
12. Using the Datapoint Browser Widget, you can browse the datapoint values.  If you are using the attached
    exampl_xif.xpl XIF file, this example/ETI driver specific rule apply:
	  * Device's unid (Name under CMS Devices Widget) is mapped to ETI's %dev_uid
	  * Datapoint name designation RO=ReadOnly and RW-Read/Write access
	  * In ETI, Address field in XIF file designate the device reg index and mapped to ETI's $reg_index.  (This 
	    mapping of Address field to device's reg index is only specific to ETI implementation.  In other driver
		implementations, the Address field may be used differently such as to map a given Device's unid to device's 
		I/O port via the Address field.)
	  * Datapoints with the same Address field share the same reg index: one designated with RW and the other RO
	  * COUNTER_RO and COUNTER_RW with Address 0 are mapped to physical address/reg 0
	  * COUNTER_RO gets its value via the use of XIF Address/Reg & custom/unrecognized column "TestMultiplier".  It
	    basically takes the value written by COUNTER_RW in Reg 0 and multiply it by the value in "TestMultiplier".
	  * Counter2_RO with Address 1 is mapped to physical address/reg 1.  We can use the ETI protocol to update the
	    value of Counter2 by publishing the following MQTT topic: 
		   mosquitto_pub -t eti/0/ev/dev/1003/reg/1 -m 30
	  * Switch1_RO and Switch1_RW with Address 2 are mapped to physical address/reg 2
	  * SwitchValue_RO and SwitchValue_RW with Address 3 are mapped to physical address/reg 3
	  * SwitchState_RO and SwitchState_RW with Address 4 are mapped to physical address/reg 4
	  * All datapoint read from the Datapoints Browser Widget results in the following rd MQTT topic publications:
	       eti/0/rd/dev/$dev_uid/reg/$reg_index
		NOTE: these read MQTT topic publication will not affect or cause datapoint value update
	  * All datapoint write from the Datapoint Browser Widget results in the following wr MQTT topic publications:
	       eti/0/wr/dev/$dev_uid/reg/$reg_index <data payload>
	  * All externally generated ETI MQTT ev topic publications as shown below will results in datapoint updates:
	       eti/0/ev/dev/$dev_uid/reg/$reg_index <data payload>
	  * <data payload> will be in the JSON form like:
	    real number value (0, 1, 0.5, 95.5, etc.)
		boolean (true, false)
		objects ({"state":value,"value":value}, {"Name": "Apple","Price": 3.99,"Sizes":"Small"}, etc.)
 


