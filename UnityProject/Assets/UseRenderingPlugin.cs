using UnityEngine;
using System;
using System.Collections;
using System.Runtime.InteropServices;
using UnityEngine.Rendering;
using System.IO;

public class UseRenderingPlugin : MonoBehaviour
{
#if (PLATFORM_IOS || PLATFORM_TVOS || PLATFORM_BRATWURST || PLATFORM_SWITCH) && !UNITY_EDITOR
    [DllImport("__Internal")]
#else
    [DllImport("RenderingPlugin")]
#endif
    private static extern IntPtr EncodeJPEG(IntPtr textureHandle, int textureWidth, int textureHeight, int rowPitch, out long length);
#if (PLATFORM_IOS || PLATFORM_TVOS || PLATFORM_BRATWURST || PLATFORM_SWITCH) && !UNITY_EDITOR
    [DllImport("__Internal")]
#else
    [DllImport("RenderingPlugin")]
#endif
    private static extern void SetLog(bool enable,string path);
#if (PLATFORM_IOS || PLATFORM_TVOS || PLATFORM_BRATWURST || PLATFORM_SWITCH) && !UNITY_EDITOR
    [DllImport("__Internal")]
#else
    [DllImport("RenderingPlugin")]
#endif
    private static extern void ClearCache();


    public RenderTexture camRenderTex;
    public Color color1 = Color.red;
    public Color color2 = Color.blue;
    IEnumerator Start()
    {

        CreateTextureAndPassToPlugin();
        yield return StartCoroutine("CallPluginAtEndOfFrames");
    }
    void OnDisable()
    {
    }
    private void Update()
    {
        float t = Mathf.PingPong(Time.time / 2.0f, 1.0f);
        GameObject.FindGameObjectWithTag("Light").GetComponent<Light>().color = Color.Lerp(color1, color2, t);
    }
    private void CreateTextureAndPassToPlugin()
    {
        RenderTexture.active = camRenderTex;
        var cameras = Camera.allCameras;
        int i = 0;
        foreach (Camera camera in cameras)
        {
            if (camera.tag != "UnitCamera") continue;

            camera.targetTexture = camRenderTex;
            int totalCol = 4, totalRow = 5;
            int col = i % totalCol;//0,1,2,3
            int row = i / totalCol;//0,1,2,3,4
            camera.rect = new Rect(
                col * 1.0f / totalCol,
                row * 1.0f / totalRow,
                1.0f / totalCol,
                1.0f / totalRow);
            i++;
        }
        return;
    }
    private IEnumerator CallPluginAtEndOfFrames()
    {
        int captureCounter = 0, captureInterval = 1; // meaning encode every frame with renderer thread
        while (true)
        {
            // Wait until all frame rendering is done
            yield return new WaitForEndOfFrame();
            if (SystemInfo.graphicsDeviceType == GraphicsDeviceType.Direct3D11)
            {
                if (++captureCounter >= captureInterval)
                {
                    long length;
                    IntPtr dataPtr = EncodeJPEG(camRenderTex.GetNativeTexturePtr(), camRenderTex.width, camRenderTex.height, camRenderTex.width * 4, out length); //dataPtr is avaliable until next invoke this function
                    //Debug.Log("Length is " + length);
                    byte[] data = new byte[length];
                    Marshal.Copy(dataPtr, data, 0, (int)length);
                    // use code below to save a jpeg pic to disk


                    //using (FileStream fileStream = new FileStream("D:\\unity.jpeg", FileMode.Create))
                    //{
                    //    using (BinaryWriter writer = new BinaryWriter(fileStream))
                    //    {
                    //        writer.Write(data);
                    //    }
                    //}
                    captureCounter %= captureInterval;
                }

            }
        }
    }
}
