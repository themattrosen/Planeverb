using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class MiniMovmentScript : MonoBehaviour
{
	public float TimeFactor;
	public float DistanceFactor;
	public bool FreezeX;
	public bool FreezeY;
	public bool FreezeZ;


    // Start is called before the first frame update
    void Start()
    {
    }

    // Update is called once per frame
    void Update()
    {
		float add = Mathf.Sin(Time.time * TimeFactor) * DistanceFactor;
		Vector3 pos = transform.position;

		if (!FreezeX)
			pos.x += add;
		if (!FreezeY)
			pos.y += add;
		if (!FreezeZ)
			pos.z += add;

		transform.position = pos;
    }
}
