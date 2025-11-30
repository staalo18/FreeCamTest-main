namespace FCSE {
    class FreeCameraManager {
        public:
            static FreeCameraManager& GetSingleton() {
                static FreeCameraManager instance;
                return instance;
            }
            FreeCameraManager(const FreeCameraManager&) = delete;
            FreeCameraManager& operator=(const FreeCameraManager&) = delete;

            void Update();

            void TransitionTo(RE::NiPoint3 a_targetPos, float a_targetYaw, float a_transitionTime);

            void FindTarget();

            void ToggleFreeCamera();

            void Activate(bool  a_activate);

            bool IsActive() const;

            bool IsCameraLocked();

            void SetUserTurning(bool a_turning);

            bool GetFreeCameraToggled() const;

            void SetFreeCameraToggled(bool a_toggled);

            void DetachTarget();

        private:
            FreeCameraManager() = default;
            ~FreeCameraManager() = default;

            RE::BSTPoint2<float> ComputeRotation(RE::NiPoint3& a_transitionVec, float a_transitionDelta, RE::BSTPoint2<float>& a_currentRotation, RE::BSTPoint2<float>& a_targetRotation);

            RE::NiPointer<RE::NiAVObject> GetTargetPoint(RE::Actor* a_actor) const;
            
            RE::FreeCameraState* GetFreeCameraState() const;

            float ComputeAngleStep(float a_transitionDelta, float a_transitionStart, float a_transitionEnd, float a_currentAngle, float a_targetAngle);
            
            // members
            RE::Actor* m_target = nullptr;
            bool m_isAttachedToTarget = false;
            bool m_isFreeCameraActive = false;
            bool m_isFreeCameraToggled = false;
            float m_yawOffset = 0.0f;
            bool m_cameraLocked = false;
            bool m_userTurning = false;

            float m_percentageTransitioned = 0.0f;
            bool m_isTransitioning = false;

    }; // class FreeCameraManager
} // namespace FCSE
