using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class PlayerBehavior : MonoBehaviour
{
	public CharacterController controller;
	public float speed = 12f;
	public float jumpHeight = 1f;

	public float gravity = -9.8f;

	public Transform groundCheck;
	public float groundDistance = 0.4f;
	public LayerMask groundMask;

	private Vector3 velocity;
	private bool isGrounded;

	private float originalHeight;
	public KeyCode crouchButton = KeyCode.LeftControl;
	public float crouchHeight;
	public float crouchSmoothingFactor = 0.5f;

    // Start is called before the first frame update
    void Start()
    {
		originalHeight = controller.height;
    }

    // Update is called once per frame
    void Update()
    {
		isGrounded = Physics.CheckSphere(groundCheck.position, groundDistance, groundMask);
		if(isGrounded && velocity.y < 0f)
		{
			velocity.y = -2f;
		}

		float x = Input.GetAxis("Horizontal");
		float z = Input.GetAxis("Vertical");

		Vector3 move = transform.right * x + transform.forward * z;
		float shiftFactor = 1f;
		if(Input.GetKey(KeyCode.LeftShift))
		{
			shiftFactor = 1.5f;
		}
		controller.Move(move * speed * shiftFactor * Time.deltaTime);

		velocity.y += gravity * Time.deltaTime;
		controller.Move(velocity * Time.deltaTime);

		if(Input.GetButtonDown("Jump") && isGrounded)
		{
			float factor = jumpHeight * -2f * gravity;
			velocity.y = factor;
		}

		// handle crouch lerp every frame
		if(Input.GetKey(crouchButton))
		{
			Crouch();
		}
		else
		{
			Uncrouch();
		}
    }

	void Crouch()
	{
		controller.height = Mathf.Lerp(controller.height, crouchHeight, crouchSmoothingFactor);
	}

	void Uncrouch()
	{
		controller.height = Mathf.Lerp(controller.height, originalHeight, crouchSmoothingFactor);
	}
	
}
