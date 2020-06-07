using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class SoundCueController : MonoBehaviour
{
	public KeyCode advancer = KeyCode.P;
	public List<Planeverb.PlaneverbEmitter> orderedCues = new List<Planeverb.PlaneverbEmitter>();
	private int cueIndex = 0;

    // Start is called before the first frame update
    void Start()
    {
		Debug.Assert(orderedCues.Count > 0);
    }

    // Update is called once per frame
    void Update()
    {
        if(Input.GetKeyUp(advancer))
		{
			Planeverb.PlaneverbEmitter next = orderedCues[cueIndex++];
			next.Emit();
		}
    }
}
