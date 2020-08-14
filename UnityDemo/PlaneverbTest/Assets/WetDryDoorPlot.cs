using System.Collections;
using System.Collections.Generic;
using System.Text;
using System.IO;
using UnityEngine;

public class WetDryDoorPlot : MonoBehaviour
{
	public Planeverb.PlaneverbEmitter Emitter = null;
	public Transform Door = null;
	public KeyCode KeyTrigger = KeyCode.O;
	public string DirectoryName;
	public string FileName;
	public float DoorMinZ = 0f;
	public float DoorMaxZ = 0f;
	public float PauseTimeMs = 250f;

	private float[] wetGainData = null;
	private float[] dryGainData = null;
	private float[] doorData = null;
	private int dataCount = 0;
	private bool isRunning = false;
	private float doorZIncrement;
	private float timeSinceLastMove = 0f;

    // Start is called before the first frame update
    void Start()
    {
		int arraySize = 101;
		wetGainData = new float[arraySize];
		dryGainData = new float[arraySize];
		doorData = new float[arraySize];
		doorZIncrement = (DoorMaxZ - DoorMinZ) / 100f;
		ClearState();
    }

	void ClearState()
    {
		isRunning = false;
		Vector3 position = Door.position;
		position.z = DoorMinZ;
		Door.position = position;
		timeSinceLastMove = 0;
		dataCount = 0;
	}

    // Update is called once per frame
    void Update()
    {
		if (isRunning)
		{
			if (timeSinceLastMove * 1000 < PauseTimeMs)
			{
				timeSinceLastMove += Time.deltaTime;
			}
			else
			{
				// Record data
				int id = Emitter.GetID();
				var pvOutput = Planeverb.PlaneverbContext.GetOutput(id);
				wetGainData[dataCount] = pvOutput.wetGain;
				dryGainData[dataCount] = pvOutput.occlusion;
				doorData[dataCount] = dataCount;

				// Move door and pause
				timeSinceLastMove = 0;
				Vector3 position = Door.position;
				position.z += doorZIncrement;
				Door.position = position;

				// Check for completion
				dataCount++;
				if (dataCount >= doorData.Length)
				{
					ClearState();
					Debug.Log("WetDryDoor Data Logging Ended");
				}
			}
		}
		else if(Input.GetKeyDown(KeyTrigger))
		{
			ClearState();
			isRunning = true;
			Debug.Log("WetDryDoor Data Logging Started");
		}
    }

	private void OnApplicationQuit()
	{
		string path = DirectoryName + FileName;
		StringBuilder fileContents = new StringBuilder();
		int arraySize = wetGainData.Length;

		fileContents.Append("Door Percent Closed, Dry Gain, Wet Gain\n");

		for(int i = 0; i < arraySize; ++i)
		{
			fileContents.AppendFormat("{0}, {1}, {2}\n", doorData[i], dryGainData[i], wetGainData[i]);
		}

		File.WriteAllText(path, fileContents.ToString());		
	}
}
