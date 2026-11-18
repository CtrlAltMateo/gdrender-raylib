#include "AudioEngine.h"

#include <fstream>
#include <streambuf>
#include <iostream>

AudioEngine::~AudioEngine()
{
	if (isPlaying)
	{
		PauseMusicStream(music);
	}

	if (IsMusicValid(music))
	{
		UnloadMusicStream(music);
	}
}

std::optional<AudioEngine> AudioEngine::create()
{
	std::optional<AudioEngine> ret{std::in_place}; //avoids temporary
	return ret->init() ? ret : std::nullopt;
}

bool AudioEngine::init()
{
	if (IsAudioDeviceReady())
		return true;

	InitAudioDevice();

	return IsAudioDeviceReady();
}

void AudioEngine::update()
{
	if (isPlaying)
	{
		UpdateMusicStream(music);
	}
}

bool AudioEngine::loadAudio(const std::filesystem::path& filePath)
{
	if (!std::filesystem::exists(filePath))
		return false;

	if (IsMusicValid(music))
	{
		UnloadMusicStream(music);
	}

	music = LoadMusicStream(filePath.string().c_str()); //TODO: read file manually to fix unicode paths
	return IsMusicValid(music);
}

void AudioEngine::play()
{
	if (!IsMusicStreamPlaying(music) && IsMusicValid(music))
	{
		PlayMusicStream(music);
		isPlaying = true;
	}
}

void AudioEngine::pause()
{
	if (IsMusicStreamPlaying(music))
	{
		PauseMusicStream(music);
		isPlaying = false;
	}
}

void AudioEngine::resume()
{
	if (!IsMusicStreamPlaying(music))
	{
		ResumeMusicStream(music);
		isPlaying = true;
	}
}

void AudioEngine::setPosition(float seconds)
{
	SeekMusicStream(music, seconds);
}

void AudioEngine::setVolume(float volume)
{
	SetMusicVolume(music, volume);
}
