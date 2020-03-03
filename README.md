# Maya_External_Client_Plug-in
A school assignment "Implement a real-time communication mechanism between Maya and an external application using message passing through shared memory. The external application will be a rendering engine with basic support for rendering such as transforms, meshes, lights, materials and camera movements."

The plug-in is made to function with the "Gameplay3D" game engine. However as the information is passed to the engine through a circular buffer in shared memory with use of a custom-made "Comlib" the plug-in could concievably be quickly refactored to function with almost any game engine provided that it handles vertices in an industry-standard way.

The application is not yet user-friendly and is mostly intended for my own personal use. I do intend to make it a proper plug-in accesable with just a button press in Maya but I will not have the time to do so for another few months I fear.
However if you wish to run the plug-in anyway follow these instructions:

1. Download the GitHub zip and unpack.

2. Open the files MayaApi.sln and MayaViewer.sln

3. Open Maya 2019 (Please note that the Plug-in is only guranteed to function with VS2017 and Maya2019)

4. Please Ensure that you have the correct filepath in the loadPlugin.py file on line 23.
it should be <"filePath to unpack location"> /GamePlay3DExample/MayaPlugin/x64/Release/MayaAPI.mll")\n'
Otherwise building the MayaAPI solution later will erronously fail with:
Error	MSB3073	The command "python unloadPlugin.py

5. In a MEL tab in Maya's script editor enter: commandPort -n ":1234"
and run it.

6. Build the MayaAPI solution

7. Run the MayaViewer solution, Gameplay3D window opens.

8. Start Manipulating meshes and transforms in maya, they should appear in the gameplay3D window and react in real time.
