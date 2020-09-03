# Simple-ECS C++20 library
![Build & Run Test](https://github.com/DNKpp/Simple-ECS/workflows/Build%20&%20Run%20Test/badge.svg)

## Author
Dominic Koepke  
Mail: <DNKpp2011@gmail.com>

## License

[BSL-1.0](https://github.com/DNKpp/CitiesSkylines_AdvancedOutsideConnection/blob/master/LICENSE_1_0.txt) (free, open source)

```text
          Copyright Dominic Koepke 2020 - 2020.
 Distributed under the Boost Software License, Version 1.0.
    (See accompanying file LICENSE_1_0.txt or copy at
          https://www.boost.org/LICENSE_1_0.txt)
```

## Description
This header-only library provides simple tools for managing a lightweight Entity Component System environment. It is currently under development and should therefore
be used with caution. You can use doxygen to extract the documentation out of the header files.

## Simple usage example
```cpp
#include <Simple-ECS/Simple-ECS.hpp>

secs::World globalWorld;  // should only be created once during program runtime

struct MyComponent
{
    int myData = 0;
};

// There are a few more virtual functions you can override
class MySystem :
    public secs::SystemBase<MyComponent>
{
public:
    void update(float delta) override
    {
        forEachComponent([](secs::Entity& entity, MyComponent& component) { component.myData += 1; });
    }

    void postUpdate()
    {
        forEachComponent([](secs::Entity& entity, MyComponent& component)
        {
            if (100 < component.myData)
            {
                globalWorld.destroyEntityLater(entity.uid());
            }
        });
    }
};

int main()
{
    globalWorld.registerSystem(MySystem{});     // registers a unique System, which components we'll be able to instantiate
    globalWorld.createEntity<MyComponent>();    // creates an Entity which World will take care of

    for (int i = 0; i < 1000; ++i)
    {
        // these three functions need to be called each tick in exactly this order.
        globalWorld.preUpdate();
        globalWorld.update(1000.f / 60.f);  // let's simply fake a delta for each cycle
        globalWorld.postUpdate();
    }
}
```
