# XR Home Suite Deep Architecture Research Prompt

Copy and paste everything below into a research-capable LLM. The goal is to get a current, source-backed architecture recommendation for the real XR Home Suite product, ranked from best to worst, not a shallow "Unity vs Unreal" comparison.

```text
You are a senior XR/graphics/runtime architect. I need a deep, evidence-backed stack recommendation for a PC-hosted mixed-reality home environment called XR Home Suite.

Your job:

1. Research current official sources, engineering docs, SDK docs, release notes, forum evidence from engine/runtime maintainers, and credible developer reports.
2. Decide the best foundational architecture, runtime, engine, graphics API, rendering path, plugin stack, and integration strategy.
3. Rank multiple viable architecture ideas from best to worst for this exact project.
4. Be explicit about what is supported today, what is experimental, what is only plausible, and what is blocked by Meta/NVIDIA/engine/runtime limitations.
5. Optimize for using an NVIDIA GeForce RTX 4070 as fully as possible without destroying VR comfort, latency, or stability.
6. Do not assume access to unrestricted Quest camera frames unless official/current sources prove it for the selected PC-hosted path.
7. Do not recommend rooting, OS replacement, or private reverse engineering as the main product path unless you clearly separate it as a risky research-only track.

Current date context:

- Treat current information as needing verification. Check current docs and release notes, especially for NVIDIA DLSS/Streamline, Unreal/NvRTX, Unity HDRP/XR, Meta Horizon Link/OpenXR, and Meta Quest passthrough/camera APIs.
- If current docs conflict with older forum comments, prefer official docs, then maintainer comments, then reproducible community evidence.
- Use dates on sources whenever possible.

Project objective:

- Build an actual XR Home Suite: a PC-hosted, high-performance replacement for the stale Meta Quest Link home experience.
- The app should run on the Windows host PC and stream/render to a Meta Quest 3 through Meta Horizon Link / Quest Link.
- The app should keep Meta Horizon Link / Oculus OpenXR runtime underneath for device transport, compositor, tracking, passthrough, hand tracking, depth/spatial APIs, and Link compatibility.
- The app should feel like a real modern XR home shell: passthrough background, high-quality 3D home space, spatial launcher, hands/controllers, panels, previews, room-aware placement, performance overlay, and eventually AI/CV features if camera access is possible through supported APIs.
- The app should use the RTX 4070 heavily for rendering, upscaling, ray tracing/neural rendering when appropriate, and low-latency GPU work.

Hardware and environment:

- Host OS: Windows 10/11.
- GPU: NVIDIA GeForce RTX 4070.
- Headset: Meta Quest 3, possibly Quest 3S.
- Transport: Meta Horizon Link / Quest Link, USB 3.x preferred. Air Link may be tested but should not be the performance baseline.
- Existing observed OpenXR runtime: Oculus OpenXR 1.205.0.
- Existing observed Meta Horizon Link version from this project context: 205.0.0.103.543.
- Meta Horizon Link is set as the active OpenXR runtime.
- Meta Horizon Link developer toggles enabled on the test machine:
  - Developer Runtime Features: on.
  - Passthrough over Meta Horizon Link: on.
  - Passthrough Camera API permissions: on.
  - Spatial Data over Meta Horizon Link: on.
  - Eye tracking over Meta Horizon Link: off, ignore for Quest 3.
  - Natural Facial Expressions over Meta Horizon Link: off, ignore for Quest 3.

Existing repo and prototype context:

- Repo: XR Home Suite.
- Current prototype is a native Windows C++ OpenXR app.
- Current graphics path: D3D11 via XR_KHR_D3D11_enable.
- Current build intentionally avoids Java, Gradle, Unity, Android Studio, APK deploy loops, and any Android-only app path for the PC-hosted test.
- Reason: earlier Java/managed/editor paths felt laggy; the desired product should not be CPU-bound or dependent on slow build/deploy loops.
- Current prototype entrypoint: apps/openxr_probe.
- Current native code is split across libs/xrh_core, libs/xrh_openxr, libs/xrh_d3d11, libs/xrh_meta, and apps/openxr_probe.
- Current orchestration uses Python + CMake presets. Heavy dependencies and build output live outside the repo under XRHS_HOME, defaulting to C:\XRHomeSuite.
- Current run flags:
  - sync dependencies: python -m xrhs deps sync
  - configure: python -m xrhs configure --preset native-vs-debug
  - build: python -m xrhs build --preset native-vs-debug
  - render-only lag isolation: python -m xrhs run-probe -- --seconds 60 --no-depth --no-hands
  - hands only: python -m xrhs run-probe -- --seconds 60 --no-depth --hand-hz 15
  - conservative hands + depth: python -m xrhs run-probe -- --seconds 60 --depth-hz 15 --hand-hz 15
  - optional JSON report: python -m xrhs run-probe -- --seconds 5 --no-depth --no-hands --report C:\XRHomeSuite\artifacts\reports\render-only.json
- Current prototype has graceful Ctrl+C shutdown:
  - Windows console Ctrl+C requests a clean OpenXR shutdown.
  - It avoids abrupt process termination mid-frame.
  - It finishes or avoids new frame submission, pauses/destroys passthrough, stops depth, destroys hand trackers, destroys OpenXR session and instance.

Verified local prototype findings:

- Runtime: Oculus OpenXR 1.205.0.
- GPU selected by runtime: NVIDIA GeForce RTX 4070.
- Eye/color swapchains observed: 2720x2976 per eye, 3 images per eye.
- OpenXR color texture format observed:
  - Runtime returned typeless D3D texture format 27.
  - App render-target view uses selected concrete format 28.
- Public passthrough:
  - XR_FB_passthrough is present and works.
  - App can create and start a passthrough layer.
  - App can submit a transparent GPU-rendered overlay over passthrough.
  - Passthrough layer uses XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT.
  - XrCompositionLayerPassthroughFB::space must be XR_NULL_HANDLE in this runtime path.
  - Tested environment blend mode is XR_ENVIRONMENT_BLEND_MODE_OPAQUE.
- Passthrough capabilities observed:
  - 0x3, interpreted in this repo as base passthrough + color passthrough.
  - XR_PASSTHROUGH_CAPABILITY_LAYER_DEPTH_BIT_FB was not present in that observed value.
- Environment depth:
  - XR_META_environment_depth is present and works.
  - supportsEnvironmentDepth = true.
  - supportsHandRemoval = false on this Link session.
  - Environment depth swapchain observed: 320x320, 3 images.
  - Depth acquisition result: XR_SUCCESS.
  - Environment-depth images must be initialized/enumerated as XR_TYPE_ENVIRONMENT_DEPTH_IMAGE_VIEW_META.
  - Current app caps depth polling to 30 Hz by default.
  - Current app supports --no-depth and --depth-hz N.
- Hand tracking:
  - XR_EXT_hand_tracking is present and works.
  - Left/right hand trackers can be created.
  - Live joint frames were observed.
  - XR_HAND_JOINT_COUNT_EXT = 26 joints per hand.
  - Current app renders projected in-headset hand-joint skeleton preview.
  - Current app caps hand polling to 30 Hz by default.
  - Current app supports --no-hands and --hand-hz N.
- Private/native camera probe:
  - XR_METAX1_passthrough_camera_data is visible in the tested runtime.
  - Count-only function probe xrEnumeratePassthroughCameraSourcePropertiesMETAX1 returned XR_SUCCESS with 2 camera sources.
  - The app intentionally does not attempt raw private camera frame acquisition because the extension is private/undocumented.
  - Unknowns include struct layouts, image handles, frame buffers, timestamps, permissions, lifetime, and ABI details.
  - Guessing this private ABI could crash the runtime or read invalid memory.

Important Meta passthrough/camera distinction:

- XR_FB_passthrough:
  - Compositor-owned passthrough visualization.
  - Apps create a passthrough layer, and the XR compositor supplies/visualizes the real camera feed.
  - Apps do not receive camera pixels through this path.
  - Useful for seeing real world as background and compositing virtual content over it.
  - App-visible styling knobs include opacity, edge color, mono color map, brightness, contrast, saturation.
  - App does not control camera resolution or raw camera stream quality through this path.
- Passthrough Camera API / PCA:
  - Separate permissioned path for app access to camera data for CV/ML/custom processing.
  - Official public routes seen so far include Android Camera2, Unity/MRUK PassthroughCameraAccess, Spatial SDK, and newer Unreal extension/support.
  - Need current research to determine whether PC-hosted Meta Horizon Link supports public camera-frame access in Unreal/native/Unity today, and exactly under what stack.
  - Do not conflate "Passthrough over Meta Horizon Link" with "app gets raw 4K camera frames over USB".

Known public PCA constraints from existing project notes:

- Supported devices in official docs observed so far: Quest 3 / Quest 3S.
- Camera source: left or right forward-facing RGB camera.
- Camera data includes texture/data, intrinsics, extrinsics, timestamps.
- Supported Unity/MRUK resolutions observed in docs:
  - 320x240
  - 640x360
  - 640x480
  - 720x480
  - 720x576
  - 800x600
  - 1024x576
  - 1280x720
  - 1280x960
  - 1280x1080
  - 1280x1280
- Documented max observed so far: 1280x1280.
- Documented stream rate observed so far: 60 Hz.
- Documented format observed so far: internal YUV420.
- Documented latency observed so far: 20-40 ms.
- Documented overhead observed so far: about 1-2% GPU overhead per streamed camera and about 45 MB memory overhead.
- Important rule: do not blindly pick the largest resolution. Pick concrete tested resolutions/aspect ratios because supported resolutions can change by device/runtime/API.

Known Meta Link constraints from current notes:

- Meta says Passthrough over Link is a developer feature to speed iteration and visual/performance characteristics can differ from running on-device.
- Meta says Color Passthrough over Link should have effective USB bandwidth of at least 2 Gbps.
- Meta says camera images are transmitted to and processed on the host PC for Passthrough over Link, but this does not automatically mean the app receives raw camera image buffers.
- Meta says screenshots of passthrough apps over Link may omit passthrough background due to privacy constraints.
- Space Setup cannot be performed over Link. Room/scene scanning must happen on-device before scene data can be used over Link.
- Existing notes say Meta OS maintains up to 15 rooms.

Relevant OpenXR/native extension set currently considered:

- Required:
  - XR_KHR_D3D11_enable in current prototype.
- Optional/currently enabled in prototype:
  - XR_FB_passthrough.
  - XR_FB_triangle_mesh.
  - XR_META_environment_depth.
  - XR_FB_scene.
  - XR_FB_spatial_entity.
  - XR_META_spatial_entity_mesh.
  - XR_EXT_hand_tracking.
  - XR_METAX1_passthrough_camera_data private count-only probe.
- Potential future native graphics extension/path:
  - XR_KHR_D3D12_enable or equivalent D3D12 OpenXR path if moving to a native D3D12 renderer or engine that uses D3D12.

User's goals/preferences/constraints:

- Wants a real app, not just a toy probe.
- Wants strongest use of RTX 4070.
- Wants GPU-powered engine/runtime, not CPU-heavy lag.
- Wants to avoid relying on Java/Gradle/Android Studio/APK loops for the PC-hosted product.
- Wants immediate shutdown on Ctrl+C and stable runtime behavior.
- Wants in-built preview/debug surfaces inside the app.
- Wants hand tracking preview inside the built app.
- Wants depth understood and used where it improves mixed reality, but not at the cost of avoidable lag.
- Wants to know the actual constraints and max quality settings for every enabled XR path.
- Wants technical docs kept locally.
- Wants a foundation now: choose stack, runtime, engine, architecture, and research tracks before building the main product.

NVIDIA / RTX 4070 facts that must be verified and considered:

- RTX 4070 is Ada Lovelace, RTX 40-series.
- Need to exploit where appropriate:
  - DLSS Super Resolution.
  - DLAA.
  - DLSS Frame Generation if supported in the selected runtime/engine/rendering path.
  - NVIDIA Reflex Low Latency.
  - Ray Reconstruction if supported in the selected path.
  - RTX ray tracing cores.
  - Tensor cores.
  - NVIDIA Streamline if using custom engine or relevant plugin path.
  - NVIDIA RTXDI / NRD / RTXGI / ReSTIR features if using NvRTX branch, if current and compatible.
- Important known/likely constraints from current docs:
  - NVIDIA Streamline supports D3D11/D3D12 in official docs; D3D12 is required for DLSS Frame Generation.
  - DLSS Frame Generation works on RTX 40 and RTX 50 series.
  - DLSS Multi Frame Generation / Dynamic MFG / 6X modes are RTX 50-series-only according to recent NVIDIA docs/news, not RTX 4070.
  - Reflex is critical if using Frame Generation because incorrect Reflex integration can increase input latency.
  - VR comfort means generated frames and latency are not automatically good. Research whether DLSS FG is appropriate or problematic for OpenXR/PCVR/Quest Link and whether it works in Unreal/Unity/native VR today.
  - DLSS Super Resolution/DLAA may be useful in VR, but verify current engine plugin support and known stereo/right-eye issues.

Candidate architecture families to evaluate:

1. Unreal Engine 5.x stock + Meta XR + OpenXR + DX12 + NVIDIA DLSS plugin/Streamline.
   - Main app built in Unreal.
   - Use Meta Horizon Link/OpenXR for Quest Link PCVR.
   - Use DX12 for NVIDIA feature compatibility.
   - Use NVIDIA DLSS plugin, Reflex, DLAA, possibly Ray Reconstruction if compatible.
   - Use Unreal VR/XR features, UMG/Slate/3D widgets or custom UI, input abstraction, asset pipeline.
   - Keep current native C++ OpenXR probe as a companion diagnostic/probing tool.

2. Unreal Engine 5.x NvRTX branch + Meta/OpenXR + DX12.
   - Use NVIDIA RTX Branch for latest NVIDIA ray tracing/neural rendering features.
   - Evaluate RTXDI, NRD, RTXGI/ReSTIR, DLSS integration, Reflex, Ray Reconstruction.
   - Must evaluate compatibility with Meta XR plugin, OpenXR, Quest Link, VR stereo, packaging burden, stability, branch maintenance risk, build time, and plugin compatibility.

3. Native custom C++ engine: OpenXR + D3D12 + NVIDIA Streamline/NGX/Reflex + custom renderer/UI.
   - Maximum control and least engine overhead.
   - Best direct D3D12/OpenXR control and explicit Streamline integration.
   - Must build UI system, scene system, asset pipeline, text rendering, interaction layer, settings, debug UI, profiler, packaging, editor/tools, and content workflow.
   - Good for low-level proof and maybe long-term shell if scope remains small, but high engineering cost.

4. Hybrid: Unreal main app + native OpenXR/D3D companion service/plugin.
   - Unreal for production renderer/content/tools.
   - Native C++ module/service remains for direct OpenXR extension probes, capability matrix, diagnostics, and private ABI research isolated from product runtime.
   - Possible interprocess communication or Unreal plugin boundary.
   - Needs evaluation: can one process own OpenXR session? Should native probe be separate offline diagnostic instead of live companion? Can Unreal plugin safely call extensions not exposed by Meta plugin?

5. Unity HDRP or URP + Meta OpenXR + NVIDIA DLSS package.
   - Faster prototyping, strong Meta docs, possibly stronger official PCA route through Unity/MRUK.
   - But user dislikes managed/Java/editor lag and wants native/GPU-heavy PC path.
   - Must evaluate Unity HDRP VR/DLSS support, Quest Link passthrough support, OpenXR feature completeness, CPU overhead, plugin maturity, and PCVR latency.
   - Potential role: not main renderer, but a PCA/camera experiment track if public camera access is easier.

6. Godot/OpenXR or other lightweight engine.
   - Evaluate only if there is compelling evidence.
   - Likely weaker for NVIDIA RTX/DLSS/NvRTX/Meta PCVR feature coverage.

7. NVIDIA Omniverse/OpenUSD/Kit-style approach.
   - Evaluate for asset/scene tooling or rendering experiments only if plausible.
   - Likely too heavy/not suitable for low-latency Quest Link home shell, but research if there is a reason.

8. SteamVR/OpenVR/OpenComposite route.
   - Evaluate whether it adds value or only complicates Meta Link passthrough/spatial APIs.
   - Since Meta-specific passthrough/depth/spatial extensions require Meta runtime, likely not main path.

9. Android standalone Quest app with Meta SDK.
   - Strongest official passthrough/camera/spatial API path.
   - But does not use RTX 4070 as main renderer and reintroduces APK/mobile constraints.
   - Maybe companion app for public PCA/camera preprocessing, but not the main PC-hosted home suite.

10. Rooting Quest 3 / replacing OS / sideloading Linux / bypassing Meta restrictions.
   - Research feasibility only as a separate appendix.
   - Must explain if bootloader/root is practically possible on consumer Quest 3, whether camera firmware/drivers would be accessible, what would break, legality/warranty/account risk, and whether Link/Meta runtime would be lost.
   - Do not rank it as a normal product path unless there is strong evidence it is viable and safe.

Questions you must answer:

1. What is the best foundational stack for XR Home Suite and why?
2. Should the main app be Unreal, NvRTX Unreal, custom native C++ D3D12/OpenXR, Unity, or hybrid?
3. Which rendering API should be used: D3D11, D3D12, Vulkan, or engine-managed RHI? Explain in relation to OpenXR, Meta Link, RTX 4070, Streamline, DLSS/Reflex, and VR stability.
4. If Unreal is recommended, should it use stock UE, source-built UE, NvRTX branch, or Meta fork? What are the tradeoffs?
5. If Unreal is recommended, should the first production path use Forward Renderer/MSAA or Deferred/DX12/Lumen/Nanite/ray tracing? Can these be separated into "comfort mode" and "RTX showcase mode"?
6. Is DLSS Super Resolution currently practical in PCVR/OpenXR/Quest Link? Cite current sources and known bugs.
7. Is DLSS Frame Generation appropriate or supported for VR/OpenXR/Quest Link? If technically possible, should it be disabled by default because of latency/comfort? Explain.
8. How should NVIDIA Reflex be integrated or enabled? What can the engine handle, and what must the app explicitly configure?
9. Which NVIDIA RTX features can RTX 4070 use in this app, and which are RTX 50-only or otherwise unavailable?
10. How should the app handle passthrough in the recommended stack?
11. Can the recommended stack access raw passthrough camera frames over Meta Horizon Link today through official APIs? If yes, exactly how? If no, what is the safest alternative?
12. Should the project maintain a native probe executable alongside the engine app? If yes, define its responsibilities and boundaries.
13. What is the best architecture for modules: rendering shell, XR runtime adapter, Meta capability adapter, sensor scheduler, interaction system, spatial scene system, debug/preview UI, performance telemetry, and camera/CV research track?
14. How should depth be used without creating lag? Include recommended polling, GPU upload path, occlusion strategy, and fallbacks.
15. How should hand tracking be used? Include controller coexistence, confidence/validity handling, smoothing, polling rate, visual preview, and UI interactions.
16. How should spatial data / scene mesh / anchors be used over Link?
17. What should the first MVP build include, given the proven capabilities?
18. What should be deferred to research tracks?
19. What should be avoided because it is likely to waste time or destabilize the runtime?
20. What benchmarking/profiling setup should be used to prove GPU vs CPU bottlenecks?

Required output structure:

Start with a short answer:

- "Best overall choice: ..."
- "Best first implementation target: ..."
- "Best experimental/rendering branch: ..."
- "Avoid as primary path: ..."

Then provide a ranked list from best to worst:

For each option, include:

- Rank number.
- Architecture name.
- Stack components:
  - Engine/runtime.
  - Graphics API/RHI.
  - XR runtime/API.
  - Meta plugin/API path.
  - NVIDIA plugin/API path.
  - UI system.
  - Asset/content workflow.
  - Build/deploy workflow.
- Why it ranks here.
- RTX 4070 utilization:
  - DLSS SR/DLAA.
  - Frame Generation.
  - Reflex.
  - Ray tracing.
  - Ray Reconstruction.
  - RTXDI/NRD/RTXGI/ReSTIR if relevant.
  - VR-specific caveats.
- Meta Quest Link compatibility:
  - Passthrough compositor layer.
  - Raw PCA camera access.
  - Depth.
  - Hand tracking.
  - Scene/spatial data.
  - Link/USB constraints.
- Performance risks:
  - CPU overhead.
  - GPU overhead.
  - frame pacing.
  - latency.
  - Link encoder/transport bottlenecks.
  - memory/VRAM.
  - stereo rendering issues.
- Developer velocity:
  - iteration time.
  - tooling.
  - build complexity.
  - debugging/profiling.
  - plugin maturity.
- Failure modes and mitigations.
- What to prototype first to validate this option.
- Sources/citations.

Then provide a decision matrix:

Rows should include at least:

- Unreal stock DX12 OpenXR.
- Unreal NvRTX DX12 OpenXR.
- Unreal Meta fork if applicable.
- Native C++ D3D12 OpenXR Streamline.
- Current native C++ D3D11 OpenXR probe extended.
- Unity HDRP OpenXR Meta + NVIDIA package.
- Unity/Android or Quest standalone companion.
- Godot or lightweight engine.
- Omniverse/Kit/OpenUSD route if relevant.
- Root/custom OS route as research-only.

Columns should include:

- RTX feature coverage.
- VR latency/stability.
- Quest Link passthrough support.
- Raw camera access likelihood.
- Depth/spatial support.
- Hand tracking support.
- CPU overhead.
- GPU scalability.
- Development speed.
- Maintenance risk.
- Debuggability.
- Long-term product fit.
- Score out of 10.

Then provide:

1. Recommended target architecture.
2. Recommended repository layout.
3. Recommended MVP milestones.
4. Recommended prototype tests.
5. Recommended profiling plan.
6. Recommended "do not do yet" list.
7. Recommended research branches.
8. Concrete engine/project settings to try first.
9. Questions or unknowns that require live validation on the actual machine.

Specific architecture details I want you to propose:

- Process model:
  - one engine process only?
  - separate native diagnostic tool?
  - separate service?
  - how to avoid two processes fighting over OpenXR session ownership?
- Runtime abstraction:
  - how to wrap Meta-specific OpenXR extensions cleanly.
  - how to keep non-Meta fallbacks possible.
- Rendering:
  - comfort/default mode settings.
  - RTX showcase mode settings.
  - resolution scaling strategy.
  - swapchain/render target strategy.
  - frame timing.
  - foveated rendering options.
  - MSAA vs TAA/DLSS tradeoff.
- Passthrough:
  - layer ordering.
  - alpha composition.
  - style controls.
  - privacy limitations.
  - screenshot limitations.
- Depth:
  - how to ingest 320x320 or runtime-provided depth.
  - GPU upload/texture path.
  - temporal smoothing.
  - occlusion mask generation.
  - hand removal unavailable fallback.
  - near-field unreliability under about 0.2 m.
- Hands:
  - 26-joint skeleton.
  - validity flags.
  - smoothing and prediction.
  - pinch/ray interactions.
  - preview overlay.
  - fallback to controllers.
- Scene/spatial:
  - room scan dependency.
  - anchors.
  - semantic labels.
  - walls/floor/ceiling.
  - mesh simplification.
  - persistence.
- Camera/CV:
  - official PCA path if available.
  - if PC-hosted raw camera is not available, propose alternatives:
    - Android companion app.
    - Unity/Unreal public PCA experiment.
    - no raw camera path, use compositor passthrough + depth + scene instead.
  - never build product-critical features on private ABI until proven.
- Input:
  - hands.
  - controllers.
  - keyboard/mouse from PC if useful.
  - voice/AI optional only if it fits.
- UI:
  - in-world debug preview.
  - performance HUD.
  - settings panel.
  - sensor toggles.
  - Link/runtime status panel.
  - app launcher.
- Telemetry/profiling:
  - CPU frame time.
  - GPU frame time.
  - compositor timing.
  - Link bitrate/encoder if accessible.
  - sensor acquisition timing.
  - OpenXR result logging.
  - NVIDIA Nsight Graphics/Systems.
  - Unreal Insights if Unreal.
  - PIX if D3D12.
  - OpenXR runtime logs.

Expected recommended MVP constraints:

- Use only supported public APIs for the product path.
- Keep private camera ABI research isolated.
- Do not depend on raw camera feed for the first usable shell.
- First MVP should likely include:
  - OpenXR app through Meta Horizon Link.
  - passthrough background.
  - high-quality GPU-rendered spatial UI.
  - hand/controller interaction.
  - depth-aware occlusion experiment.
  - scene/spatial data readout if supported.
  - built-in debug/performance panel.
  - sensor toggles and polling caps.
  - settings profile for low latency vs high visual quality.
- First MVP should not require:
  - rooting Quest.
  - replacing Quest OS.
  - raw private camera frame access.
  - killing or replacing Meta Horizon Link services.
  - Android APK deploy loop for the main PC-hosted shell.

Source URLs already known and worth checking/updating:

- Meta passthrough: https://developers.meta.com/horizon/documentation/native/android/mobile-passthrough/
- Meta passthrough over Link: https://developers.meta.com/horizon/documentation/native/android/mobile-passthrough-over-link/
- Meta Depth API: https://developers.meta.com/horizon/documentation/native/android/mobile-depth/
- Meta Unity Depth API runtime notes: https://developers.meta.com/horizon/documentation/unity/unity-depthapi-xr-oculus/
- Meta native hand tracking: https://developers.meta.com/horizon/documentation/native/android/mobile-hand-tracking/
- Meta scene overview: https://developers.meta.com/horizon/documentation/native/android/openxr-scene-overview/
- Meta scene API reference: https://developers.meta.com/horizon/documentation/native/android/mobile-scene-api-ref/
- Meta Link for Unreal: https://developers.meta.com/horizon/documentation/unreal/unreal-link/
- Meta Unreal passthrough setup: https://developers.meta.com/horizon/documentation/unreal/unreal-passthrough-overview-gs/
- Meta Unreal forward renderer: https://developers.meta.com/horizon/documentation/unreal/unreal-forward-renderer/
- Meta Unity OpenXR settings: https://developers.meta.com/horizon/documentation/unity/unity-openxr-settings/
- Unity Meta OpenXR camera/passthrough docs: https://docs.unity3d.com/Packages/com.unity.xr.meta-openxr@2.2/manual/features/camera.html
- Epic Unreal OpenXR prerequisites: https://dev.epicgames.com/documentation/unreal-engine/openxr-prerequisites-in-unreal-engine
- Epic Unreal XR performance features: https://dev.epicgames.com/documentation/unreal-engine/xr-performance-features-in-unreal-engine
- Epic Unreal desktop rendering path features: https://dev.epicgames.com/documentation/unreal-engine/supported-features-by-rendering-path-for-desktop-with-unreal-engine
- Epic Unreal forward shading renderer: https://dev.epicgames.com/documentation/unreal-engine/forward-shading-renderer-in-unreal-engine
- NVIDIA Streamline: https://developer.nvidia.com/rtx/streamline
- NVIDIA Streamline getting started: https://developer.nvidia.com/rtx/streamline/get-started
- NVIDIA Streamline GitHub: https://github.com/NVIDIA-RTX/Streamline
- NVIDIA DLSS: https://developer.nvidia.com/rtx/dlss
- NVIDIA RTX Branch / NvRTX: https://developer.nvidia.com/game-engines/unreal-engine/rtx-branch
- NVIDIA Reflex SDK: https://developer.nvidia.com/performance-rendering-tools/reflex
- NVIDIA RTX games/engine feature list: https://www.nvidia.com/en-us/geforce/news/nvidia-rtx-games-engines-apps/
- Khronos OpenXR registry/headers: https://github.com/KhronosGroup/OpenXR-SDK

Research quality rules:

- Use official docs for capability claims.
- Use forum/community reports only for practical caveats and real-world bugs, clearly labeled.
- Include links for every important claim.
- Distinguish "works in flat-screen UE" from "works in VR/OpenXR" from "works over Meta Quest Link".
- Distinguish "works on Quest standalone Android" from "works on PC-hosted app through Meta Horizon Link".
- Distinguish "compositor can process camera images on host PC" from "app can receive raw camera frames".
- Distinguish "RTX 4070 supports this feature" from "this selected engine/runtime path supports this feature in PCVR".
- For any recommendation involving DLSS Frame Generation in VR, explicitly discuss latency/comfort risks and whether it should be off by default.
- For any recommendation involving Lumen/Nanite/ray tracing in VR, explicitly discuss forward vs deferred rendering, stereo support, MSAA/TAA/DLSS tradeoffs, and performance risk.
- For any recommendation involving NvRTX, explicitly discuss source build cost, branch lag, plugin compatibility, Meta XR compatibility, and whether it is better as a separate experimental branch.
- For any recommendation involving Unity, explicitly discuss managed/runtime/editor overhead, HDRP VR support, DLSS support, Meta PCA support, Quest Link support, and whether it should be only a camera/PCA research branch.
- For any recommendation involving native custom engine, explicitly discuss the engineering cost of building missing engine features.
- Do not optimize purely for peak benchmark visuals. Optimize for a shippable XR shell that is stable, low-latency, comfortable, and still makes strong use of RTX 4070.

Please be opinionated. At the end, give a final "build this first" answer with:

- exact stack,
- exact engine/runtime version recommendations if possible,
- first project settings,
- first plugins,
- first prototype test scene,
- first validation checklist,
- biggest risks,
- fallback path if the top recommendation fails.
```
