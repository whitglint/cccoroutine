# CCCoroutine

Cocos2d-x Coroutine Action for C++ Coroutines TS.

## Build

* Header only
* C++ compiler with Coroutines TS support
    * Visual C++: /await
    * Clang: -fcoroutines-ts -stdlib=libc++

## Example

```cpp
#include <CCCoroutine.h>

USING_NS_CC;

void FooLayer::bar()
{
    startCoroutine(this, barCoroutine());
}

cocos2d::Coroutine FooLayer::barCoroutine()
{
    // Will wait until next frame
    co_yield nullptr;
    // Will wait until the DelayTime action finish
    co_yield DelayTime::create(1000);
}
```

## License

MIT
