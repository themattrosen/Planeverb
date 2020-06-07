using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class PlayerPickup : MonoBehaviour
{
	public float PickupDistance = 0.5f;
	public float HoldDistance = 0.3f;
	public KeyCode PickupKey = KeyCode.Z;
	public int MouseButton = 0;

	private bool isGrabbing = false;
	private Transform heldObject = null;

    // Start is called before the first frame update
    void Start()
    {
        
    }

    // Update is called once per frame
    void Update()
    {
		bool keyHeld = false;
		if(PickupKey != KeyCode.None)
		{
			keyHeld = Input.GetKey(PickupKey);
		}
		else
		{
			keyHeld = Input.GetMouseButton(MouseButton);
		}

		// case just tried to grab something
		if (keyHeld && !isGrabbing)
		{
			RaycastHit hit;
			Ray ray = new Ray(transform.position, transform.forward);
			if(Physics.Raycast(ray, out hit, PickupDistance))
			{
				if (hit.transform.gameObject.GetComponent<Grabbable>())
				{
					isGrabbing = true;
					heldObject = hit.transform;
					var rbody = hit.rigidbody;
					if(rbody)
					{
						rbody.useGravity = false;
						rbody.freezeRotation = true;
					}
				}
			}
		}
		// case was grabbing something, but just stopped pressing pickup key
		else if(!keyHeld && isGrabbing)
		{
			var rbody = heldObject.gameObject.GetComponent<Rigidbody>();
			if (rbody)
			{
				rbody.useGravity = true;
				rbody.freezeRotation = false;
			}
			heldObject = null;
			isGrabbing = false;
		}

		if(isGrabbing)
		{
			HandleHeldObject();	
		}
    }

	void HandleHeldObject()
	{
		heldObject.position = transform.position + transform.forward * HoldDistance;
		
		if(Input.GetKeyUp(KeyCode.LeftArrow))
		{
			heldObject.Rotate(new Vector3(0, 90, 0));
		}
		else if(Input.GetKeyUp(KeyCode.RightArrow))
		{
			heldObject.Rotate(new Vector3(0, -90, 0));
		}

	}
}
