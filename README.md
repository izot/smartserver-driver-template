# smartserver-driver-template
Example custom driver template for the Dialog SmartServer IoT Edge Server (Dialog Semiconductor is a Renesas Electronics company). Documentation is available at http://iecdocs.renesas.com.

Create and test a custom driver for the SmartServer IoT by following these steps:

1.  Download and install Visual Studio Code(VSCode) on an ARM Linux development host.
2.  Install the recommended Extensions in Marketplace:
		 C/C++, C/C++ Extension Pack, GitHub Pull Requests and Issues, and Makefile Tools
3.  Clone the IDL example at this URL: git@github.com:izot/smartserver-driver-template.git.
4.  Using VSCode open the Makefile and change the driver to a driver-specific information :
	   The driver identifier, driver name, description, manufacturer, license, version, filetype, an extension.
5.  Add any required additional components such as a protocol stack to the build script.
6.  Complete the implementation of the required call-back functions.
7.  Click the Makefile Tools icon in VSCode left explorer menu to build the new driver.  
	   The output of the build process will be a GLPO file with an installation script and driver image embedded in the file.
8.  Create a new driver-specific XIF file by copying and modifying the exampledevice.csv file in the smartserver-driver-template project.
9.  Open the SmartServer Devices widget, select the Segment Controller tab, and then: 
    * Click the vertical three dots next to the SmartServer to select the Update menu.
    * Drag the newly created GLPO file into the Update Loader (Drop new update loader here) box.
    * Click the file icon to save the driver package into the CMS and click the import icon to load the driver to the SmartServer.
	    The new driver will be loaded and running as service (cdriver:<your driver identifier>) under Supervisorctl.
10. Using the SmartServer Device Types widget: 
	  * Click the Import device type icon and then drag your driver-specific XIF file to the 
      "DROP FILE HERE" box and click the IMPORT FILE button to load the XIF file to the SmartServer.
    * Click the Back button and wait until the device type shows up in the Device Types widget.
11. Open the SmartServer Devices widget, select the Edge Devices tab, and then:
	  * Click the "+" button to create a device with the following info:
		  name: <your device name>_1,  UID: 001, Integration Method: Manual assignment, Driver: <your driver identifier>,
		  and select the one and only available device types and click the SAVE button.
	    Wait until the newly created device is displayed in the Devices widget and shown in blue (not purple color).
    * Click the three vertical dots next to the newly created device and select Provision menu.
12. Using the Datapoints widget, browse the datapoint values.  Per XIF file, the COUNTER_A and COUNTER_B are read-only datapoints
	  and COUNTER_C and COUNTER_D are read/write datapoints.

