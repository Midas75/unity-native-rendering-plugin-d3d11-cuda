using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class AllCameraRotate : MonoBehaviour
{
    // Start is called before the first frame update
    void Start()
    {

    }

    // Update is called once per frame
    void Update()
    {
        foreach (Camera camera in Camera.allCameras)
        {
            if (camera.tag != "UnitCamera")
            {
                continue;
            }
            Vector3 currentRotation = camera.transform.rotation.eulerAngles;

            currentRotation.y += 2;

            camera.transform.rotation = Quaternion.Euler(currentRotation);
        }
    }
}
