#pragma once

namespace dmge
{
	class APUStream : public IAudioStream
	{
	public:
		APUStream();

		virtual ~APUStream();

		void pushSample(float left, float right);

		int bufferRemain() const;

		int bufferMaxSize() const;

	private:
		virtual void getAudio(float* left, float* right, size_t samplesToWrite) override;

		virtual bool hasEnded() override;

		virtual void rewind() override;

	private:
		Wave wave_;

		int posHead_ = 0;
		int posTail_ = 0;

	};
}
