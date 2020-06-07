using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class MouseLook : MonoBehaviour
{
	public float MouseSensitivity = 100;

	private Transform body;
	private float xRotation = 0f;

    // Start is called before the first frame update
    void Start()
    {
		body = transform.parent;
		Cursor.lockState = CursorLockMode.Locked;
    }

    // Update is called once per frame
    void Update()
    {
		float mouseX = Input.GetAxis("Mouse X") * MouseSensitivity * Time.deltaTime;
		float mouseY = Input.GetAxis("Mouse Y") * MouseSensitivity * Time.deltaTime;

		xRotation -= mouseY;
		xRotation = Mathf.Clamp(xRotation, -90f, 90f);

		transform.localRotation = Quaternion.Euler(xRotation, 0f, 0f);
		body.Rotate(Vector3.up * mouseX);
	}
}
