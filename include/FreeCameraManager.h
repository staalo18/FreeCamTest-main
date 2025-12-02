namespace FCSE {
    
    enum FreeCameraTransitionMode {
        kNone = 0,
        kToTarget = 1,
        kToPrevious = 2
    };

    class FreeCameraManager {
        public:
            static FreeCameraManager& GetSingleton() {
                static FreeCameraManager instance;
                return instance;
            }
            FreeCameraManager(const FreeCameraManager&) = delete;
            FreeCameraManager& operator=(const FreeCameraManager&) = delete;

            void Update();

            void SelectTarget();

            void ToggleFreeCamera();

            void SetActive(bool  a_activate);

            bool IsActive() const;

            void SetUserTurning(bool a_turning);

            bool GetFreeCameraToggled() const;

            void SetFreeCameraToggled(bool a_toggled);

        private:
            FreeCameraManager() = default;
            ~FreeCameraManager() = default;

            float ComputeTransitionTime(float a_minDist, float a_maxDist, float a_minTime, float a_maxTime);

            void TransitionTo(RE::NiPoint3 a_targetPos, RE::BSTPoint2<float> a_targetRotation, 
                float a_transitionTime, float a_rotationToMovement_End, float a_rotationToTarget_Start);

            void TransitionToTarget();

            void TransitionToPrevious();

            void UpdateFreeCamera();

            RE::BSTPoint2<float> ComputeRotation(RE::NiPoint3& a_transitionVec, float a_transitionDelta, 
                RE::BSTPoint2<float>& a_currentRotation, RE::BSTPoint2<float>& a_targetRotation,
                float a_rotationToMovement_End, float a_rotationToTarget_Start);

            RE::FreeCameraState* GetFreeCameraState() const;

            float ComputeAngleStep(float a_transitionDelta, float a_rotationStart, float a_rotationEnd, float a_currentAngle, float a_targetAngle);
            
            void StartFreeCameraMode();

            void StopFreeCameraMode();

            // members
            RE::CameraState m_previousCameraState;
            RE::NiPoint3 m_previousCameraPos;
            RE::NiPoint2 m_prevFreeRotation;
            RE::Actor* m_target = nullptr;
            float m_prevYaw= 0.0f;
            float m_prevPitch = 0.0f;
            bool m_isFreeCameraActive = false;
            bool m_isFreeCameraToggled = false;
            float m_yawOffset = 0.0f;
            bool m_userTurning = false;

            float m_percentageTransitioned = 0.0f;
            FreeCameraTransitionMode m_transitionMode = FreeCameraTransitionMode::kNone;

    }; // class FreeCameraManager
} // namespace FCSE
