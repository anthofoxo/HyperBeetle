#pragma once

#include <memory>

namespace hyperbeetle {

	class StateManager;

	class State {
	public:
		State() = default;
		virtual ~State() noexcept = default;
		virtual void Init() {}
		virtual void Update() = 0;
		virtual bool UseDebugGui() { return false; }
		StateManager& Get() const;
	protected:
		State(State const&) = default;
		State& operator=(State const&) = default;
		State(State&&) noexcept = default;
		State& operator=(State&&) noexcept = default;
	private:
		friend class StateManager;
		StateManager* mManager = nullptr;
	};

	class StateManager final {
	public:
		template<class T, class... Args>
		void Set(Args&&... args) {
			mState = std::make_unique<T>(std::forward<Args>(args)...);
			mState->mManager = this;
			mState->Init();
		}

		State* Get() { return mState.get(); }

		void SetUserPtr(void* ptr) { mUserPtr = ptr; }

		template<class T> T& GetUserPtr() { return *static_cast<T*>(mUserPtr); }

		void Update();
	private:
		std::unique_ptr<State> mState;
		void* mUserPtr;
	};

	

	inline void StateManager::Update() {
		if (mState) mState->Update();
	}

	inline StateManager& State::Get() const { return *mManager; }
}