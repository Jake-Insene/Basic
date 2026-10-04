#include "Basic/Scene/SceneManager.hpp"


namespace Basic
{

SceneManager::SceneManager(Mem::Allocator& allocator)
: allocator(allocator)
{}

SceneManager::~SceneManager()
{
    switch(state)
    {
    case Loaded:
    {
        _try_delete_current_scene();
    }
    break;
    case Transition:
    {
        _try_delete_current_scene();
        allocator.free(Mem::to_bytes(Slice(scene_state.transition.new_scene, 1)));
    }
    break;
    default:
        DebugAssert(false, "invalid state");
        break;
    }
}

void SceneManager::update(f32 dt)
{
    switch(state)
    {
    case Loaded:
    {
        current_scene->update(dt);
    }
    break;
    case Transition:
    {
        _try_delete_current_scene();

        current_scene = scene_state.transition.new_scene;
        _switch_state(Loaded);
    }
    break;
    default:
        DebugAssert(false, "invalid state");
        break;
    }
}

void SceneManager::render(Basic::SpriteBatch& sprite_batch)
{
    switch(state)
    {
    case Loaded:
    {
        current_scene->render(sprite_batch);
    }
    break;
    case Transition:
    {
        // rendering should not happend here
    }
    break;
    default:
        DebugAssert(false, "invalid state");
        break;
    }
}

void SceneManager::event(const Event& e)
{
    if(state == Null)
    {
        return; // Ignore when starting app
    }

    switch(state)
    {
    case Loaded:
    {
        current_scene->event(e);
    }
    break;
    case Transition:
    {
        // event handling should not happend here
    }
    break;
    default:
        DebugAssert(false, "invalid state");
        break;
    }
}


void SceneManager::_switch_state(State new_state)
{
    prev_state = state;
    state = new_state;
}

void SceneManager::_try_delete_current_scene()
{
    if(current_scene != nullptr)
    {
        Core::Mem::Destruct(*current_scene);
        allocator.free(Mem::to_bytes(Slice(current_scene, 1)));
    }
}

}
