# GDrender raylib

This branch contains the level renderer for embedding in RayDash. The host owns the raylib window and defines the shared `Camera2D camera`.

The host must load the required sprite atlases with `ImageLoader::ParsePlist`, set `Application::instance` and its resolution, then create a `GameLayer` with decompressed level data using `GameLayer::createWithLevelString`.

Call `Application::update` once per host frame. The renderer autostarts its level updates and advances the shared camera. `Application::draw` starts and ends the raylib drawing frame, clears it to black, and wraps the level and optional host draw callback in the shared `Camera2D camera`. The host must not wrap that call in `BeginDrawing`, `EndDrawing`, or `BeginMode2D`.

Resource paths are relative to the process working directory. The renderer uses the `Resources` folder and `CustomResources/object.json`.

Compile the renderer sources with the host and raylib. This branch has no standalone executable or window lifecycle.
