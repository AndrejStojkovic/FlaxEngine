// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

namespace FlaxEngine.GUI
{
    /// <summary>
    /// The blur panel that applied the Gaussian-blur to all content beneath the control.
    /// </summary>
    /// <seealso cref="FlaxEngine.GUI.ContainerControl" />
    public class BlurPanel : ContainerControl
    {
        /// <summary>
        /// Gets or sets the blur strength. Defines how blurry the background is. Larger numbers increase blur, resulting in a larger runtime cost on the GPU.
        /// </summary>
        [EditorOrder(0), Limit(0, 100, 0.0f)]
        public float BlurStrength { get; set; }

        /// <summary>
        /// If checked, the blur strength will be scaled with the control size, which makes it resolution-independent.
        /// </summary>
        [EditorOrder(10)]
        public bool BlurScaleWithSize { get; set; } = false;

        /// <summary>
        /// Initializes a new instance of the <see cref="BlurPanel"/> class.
        /// </summary>
        public BlurPanel()
        {
            AutoFocus = false;
        }

        /// <inheritdoc />
        public override void Draw()
        {
            base.Draw();

            var size = Size;
            var strength = BlurStrength;
            if (BlurScaleWithSize)
                strength *= size.MinValue / 1000.0f;
            if (strength > Mathf.Epsilon)
            {
                Render2D.DrawBlur(new Rectangle(Float2.Zero, size), strength);
            }
        }
    }
}
