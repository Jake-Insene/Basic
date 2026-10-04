#pragma once
#include "Basic/Scene/Scene.hpp"


namespace Basic
{

struct SceneManager
{
    DisableCopy(SceneManager);
    DisableMove(SceneManager);
    
    enum State
    {
        Null,
        Loaded,
        Unloaded,
        Transition,
    };

    Mem::Allocator& allocator;
    State state = Null;
    State prev_state = Null;
    union
    {
        struct
        {
            Scene* new_scene;
        } transition;
    } scene_state;
    
    Scene* current_scene = nullptr;

    SceneManager(Mem::Allocator& allocator);
    ~SceneManager();

    void update(f32 dt);
    void render(Basic::SpriteBatch& sprite_batch);
    void event(const Event& e);

    template<typename T, typename... TArgs>
    void load(TArgs&&... args)
    {
        scene_state.transition.new_scene = allocator.object<T>(Core::Forward<TArgs>(args)...);
        _switch_state(Transition);
    }

    void _switch_state(State new_state);
    void _try_delete_current_scene();
};

}
