/*
MIT License

Copyright (c) 2018 whitglint

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef __CCCOROUTINE_H__
#define __CCCOROUTINE_H__

#include <2d/CCAction.h>
#include <2d/CCNode.h>
#include <experimental/coroutine>
#include <memory>
#include <utility>

NS_CC_BEGIN

class Coroutine
{
public:
    class promise_type;
    using handle = std::experimental::coroutine_handle<promise_type>;
    class promise_type
    {
    public:
        ~promise_type() { CC_SAFE_RELEASE(currentAction_); }

        Action *currentAction() const noexcept { return currentAction_; }
        auto final_suspend() const noexcept { return std::experimental::suspend_always {}; }
        auto get_return_object() noexcept { return Coroutine {handle::from_promise(*this)}; }
        auto initial_suspend() const noexcept { return std::experimental::suspend_always {}; }
        auto yield_value(Action *action)
        {
            CC_SAFE_RELEASE(currentAction_);
            currentAction_ = action;
            CC_SAFE_RETAIN(currentAction_);
            return std::experimental::suspend_always {};
        }

    private:
        Action *currentAction_ {nullptr};
    };

    Coroutine() = default;
    ~Coroutine()
    {
        if (handle_) {
            handle_.destroy();
        }
    }
    Coroutine(const Coroutine &) = delete;
    Coroutine &operator =(const Coroutine &) = delete;
    Coroutine(Coroutine &&rhs) noexcept
        : handle_ {rhs.handle_}
    {
        rhs.handle_ = nullptr;
    }
    Coroutine &operator =(Coroutine &&rhs) noexcept
    {
        if (this != std::addressof(rhs)) {
            handle_ = rhs.handle_;
            rhs.handle_ = nullptr;
        }
        return *this;
    }

    Action *currentAction() const noexcept { return handle_.promise().currentAction(); }
    bool isDone() const { return handle_ && handle_.done(); }
    bool moveNext() const { return handle_ ? (handle_.resume(), !handle_.done()) : false; }

private:
    Coroutine(handle h) noexcept : handle_ {h} {}

    handle handle_;
};

class CoroutineAction : public Action
{
public:
    static CoroutineAction *create(Coroutine &&coroutine)
    {
        CoroutineAction *ret = new (std::nothrow) CoroutineAction();
        if (ret && ret->initWithCoroutine(std::forward<Coroutine>(coroutine)))
        {
            ret->autorelease();
            return ret;
        }
        delete ret;
        return nullptr;
    }

    virtual bool isDone() const override
    {
        const auto action = coroutine_.currentAction();
        if (action && !action->isDone()) {
            return false;
        }
        if (!coroutine_.isDone()) {
            return false;
        }
        return true;
    }
    virtual void step(float dt) override
    {
        auto action = coroutine_.currentAction();
        if (action && !action->isDone()) {
            action->step(dt);
            return;
        }
        coroutine_.moveNext();
    }

CC_CONSTRUCTOR_ACCESS:
    CoroutineAction() = default;
    bool initWithCoroutine(Coroutine &&coroutine) noexcept
    {
        coroutine_ = std::forward<Coroutine>(coroutine);
        return true;
    }

private:
    CC_DISALLOW_COPY_AND_ASSIGN(CoroutineAction);

    Coroutine coroutine_;
};

inline Action *startCoroutine(Node *node, Coroutine &&coroutine)
{
    return node->runAction(CoroutineAction::create(std::forward<Coroutine>(coroutine)));
}

NS_CC_END

#endif // __CCCOROUTINE_H__
