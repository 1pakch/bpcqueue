#pragma once

#include <atomic>
#include <memory>

#include <MPMCQueue.h>


namespace bpcqueue {


namespace impl {

//! Non-copyable but moveable non-owning pointer
template<typename T>
class swap_ptr {
  private:
    T* p_ = nullptr;
  public:
    swap_ptr(T* p): p_(p) {}
    swap_ptr(swap_ptr&& other): p_(other.release()) {}
    swap_ptr& operator=(swap_ptr&& other){
        p_ = other.release();
        return *this;
    }
    // Disable copying
    swap_ptr(const swap_ptr& other) = delete;
    swap_ptr& operator=(const swap_ptr& other) = delete;

    operator bool() const { return p_; }
    operator T*() const { return p_; }
    operator const T*() const { return p_; }

    T* get() { return p_; }
    
    T* reset() { p_ = nullptr; }

    T* release() {
        auto p = p_;
        p_ = nullptr;
        return p;
    }
};

}


template<typename T> class Producer;
template<typename T> class Consumer;


template<typename T>
class Queue: protected rigtorp::MPMCQueue<T> {
  private:
    std::atomic_size_t n_producers_ {0};

  protected:
    template<typename> friend class Producer;
    template<typename> friend class Consumer;

    void on_producer_new() { n_producers_.fetch_add(1, std::memory_order_release); }
    void on_producer_del() { n_producers_.fetch_sub(1, std::memory_order_release); }
    bool has_active_producer() const {
	    return n_producers_.load(std::memory_order_acquire) > 0;
    }

  public:
    using rigtorp::MPMCQueue<T>::MPMCQueue;

};


template<typename T>
class Consumer {
   private:
        Queue<T>* q_;
        bool is_empty_ = false;
        auto queue() { return q_; }
   public:
	Consumer(Queue<T>& q): q_(&q) {}
        Consumer(Consumer&&) = default;

        //! Blocking read returning false if the queue is empty.
        //! (Given that the all producers are constructed before consumers).
	bool pop(T& v) {
            while (!is_empty_) {
                auto active = queue()->has_active_producer(); 
                auto popped = queue()->try_pop(v);
                if (popped) return true;
                is_empty_ = !active && !popped;
            }
            return false;
        }
        
        auto move() { return std::move(*this); }
};


template<typename T>
class Producer {
   private:
        impl::swap_ptr<Queue<T>> q_;
        auto queue() { return q_.get(); }
   public:
	Producer(Queue<T>& q): q_(&q) { queue()->on_producer_new(); }
        Producer(Producer&&) = default;
	~Producer() { if (q_) queue()->on_producer_del(); }
        
        //! Blocks if full.
	template<typename... Args>
	void emplace(Args&&... args){
		queue()->emplace(std::forward<Args>(args)...);
	}

        //! Blocks if full.
	void push(T&& v) {
		queue()->push(std::forward<decltype(v)>(v));
	}
	
        //! Blocks if full.
        void push(T& v) {
		queue()->push(v);
	}

        auto move() { return std::move(*this); }
};

// Aliases from the client's code perspective

template<typename T> using Output = Producer<T>;
template<typename T> using Input = Consumer<T>;

template<typename T>
Producer<T> output(Queue<T>& q) { return std::move(Producer<T>(q)); }

template<typename T>
Consumer<T> input(Queue<T>& q) { return std::move(Consumer<T>(q)); }

}
