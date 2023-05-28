#include "APU.h"
#include "Memory.h"
#include "Address.h"

namespace dmge
{
	APU::APU(Memory* mem)
		: mem_{ mem }
	{
		wave_.resize(44100 / 8);
	}

	void APU::update()
	{
		if ((clock_++ % 95) > 0) return;


		// CH1

		{
			const uint8 NR11 = mem_->read(Address::NR11);
			const uint8 NR12 = mem_->read(Address::NR12);
			const uint8 NR13 = mem_->read(Address::NR13);
			const uint8 NR14 = mem_->read(Address::NR14);

			const uint8 NR21 = mem_->read(Address::NR21);
			const uint8 NR22 = mem_->read(Address::NR22);
			const uint8 NR23 = mem_->read(Address::NR23);
			const uint8 NR24 = mem_->read(Address::NR24);

			const uint8 NR30 = mem_->read(Address::NR30);
			const uint8 NR31 = mem_->read(Address::NR31);
			const uint8 NR32 = mem_->read(Address::NR32);
			const uint8 NR33 = mem_->read(Address::NR33);
			const uint8 NR34 = mem_->read(Address::NR34);

			Channel ch1{
				NR11 >> 6,
				NR11 & 63,
				NR12 >> 4,
				(NR12 >> 3) & 1,
				NR12 & 0b111,
				NR14 >> 7,
				(NR14 >> 6) & 1,
				NR13 | ((NR14 & 0b111) << 8),
			};

			Channel ch2{
				NR21 >> 6,
				NR21 & 63,
				NR22 >> 4,
				(NR22 >> 3) & 1,
				NR22 & 0b111,
				NR24 >> 7,
				(NR24 >> 6) & 1,
				NR23 | ((NR24 & 0b111) << 8),
			};

			WaveChannel ch3{
				.enable = static_cast<bool>(NR30 >> 7),
				.lengthTimer = NR31,
				.outputLevel = (NR32 >> 5) & 0b11,
				.freq = NR33 | ((NR34 &0b111) << 8),
				.enableLength = static_cast<bool>((NR34 >> 6) & 1),
				.trigger = static_cast<bool>(NR34 >> 7),
			};


			if (not ch1On_ && ch1.trigger)
			{
				ch1On_ = true;
			}
			if (ch1On_ && not ch1.trigger)
			{
				ch1On_ = false;
			}

			if (not ch2On_ && ch2.trigger)
			{
				ch2On_ = true;
			}
			if (ch2On_ && not ch2.trigger)
			{
				ch2On_ = false;
			}

			if (not ch3On_ && ch3.trigger)
			{
				ch3On_ = true;
				waveOffset_ = 0;
				Console.writeln(U"ch3 trigger");
			}
			//if (ch3On_ && not ch3.trigger)
			//{
			//	ch3On_ = false;
			//}

			double sample = 0;

			const double freq1 = 131072.0 / (2048 - ch1.freq);
			const double freq2 = 131072.0 / (2048 - ch2.freq);
			const double freq3 = 65535.0 / (2048 - ch3.freq);

			const double amplitude1 = ch1.envVol / 15.0;
			const double amplitude2 = ch2.envVol / 15.0;

			//const uint64 wavePulse1 = (clock2_ / Max(1, 44100 / 8 / (int)freq1)) % 8;
			//const uint64 wavePulse2 = (clock2_ / Max(1, 44100 / 8 / (int)freq2)) % 8;
			const uint64 wavePulse1 = (clock2_ * 8 * (int)freq1 / 44100) % 8;
			const uint64 wavePulse2 = (clock2_ * 8 * (int)freq2 / 44100) % 8;

			double sample1 = 0;
			double sample2 = 0;
			double sample3 = 0;

			switch (ch1.duty)
			{
			case 0: sample1 = wavePulse1 > 0 ? 1 : 0; break;
			case 1: sample1 = wavePulse1 > 1 ? 1 : 0; break;
			case 2: sample1 = wavePulse1 > 3 ? 1 : 0; break;
			case 3: sample1 = wavePulse1 > 5 ? 1 : 0; break;
			}

			switch (ch2.duty)
			{
			case 0: sample2 = wavePulse2 > 0 ? 1 : 0; break;
			case 1: sample2 = wavePulse2 > 1 ? 1 : 0; break;
			case 2: sample2 = wavePulse2 > 3 ? 1 : 0; break;
			case 3: sample2 = wavePulse2 > 5 ? 1 : 0; break;
			}

			sample1 = ch1On_ ? sample1 * amplitude1 * 0.33 : 0;
			sample2 = ch2On_ ? sample2 * amplitude2 * 0.33 : 0;

			int s3 = mem_->read(Address::WaveRAM + (waveOffset_ % 16)) & 0xf; // ignore left
			s3 = ch3.outputLevel == 0 ? 0 : (s3 >> (ch3.outputLevel - 1));
			s3 = ch3On_ ? s3 : 0;
			s3 = ch3.enable ? s3 : 0;

			sample3 = s3 / 15.0 * 0.33;

			sample += sample1;
			sample += sample2;
			sample += sample3;

			// 波形オフセットを周波数(freq3)分進める
			if (clock2_ % (44100 / 16 / (int)freq3) == 0)
			{
				waveOffset_++;
			}

			//ノイズ抑制？
			if (samples_ == wave_.size() - 1)
			{
				lastSample = sample;
			}
			if (samples_ == 0)
			{
				sample = (lastSample + sample) / 2;
			}

			wave_[samples_].set(sample, sample);

			samples_++;

			prevCh1_ = ch1;
			prevCh2_ = ch2;
			prevCh3_ = ch3;
		}

		if (samples_ >= wave_.size())
		{
			audioIndex_ ^= 1;

			audio_[audioIndex_] = Audio{ wave_ };
			audio_[audioIndex_].play(audioIndex_ == 0 ? MixBus0 : MixBus1);
			audio_[audioIndex_ ^ 1].pause();
		}

		if (samples_ >= wave_.size())
		{
			samples_ = 0;
		}

		clock2_++;
	}

}
