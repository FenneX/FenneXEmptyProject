/*
***************************************************************************
Copyright (c) 2013-2014 Auticiel SAS

http://www.fennex.org

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
****************************************************************************///

package com.fennex.modules;

import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.widget.Toast;
import android.content.pm.PackageManager;
import android.content.res.AssetFileDescriptor;
import android.content.res.AssetManager;
import android.media.MediaMetadataRetriever;
import android.media.MediaPlayer.OnCompletionListener;
import android.media.MediaPlayer.OnErrorListener;
import android.media.MediaPlayer.OnPreparedListener;
import android.media.MediaRecorder;
import android.media.MediaPlayer;

import org.videolan.libvlc.EventHandler;
import org.videolan.libvlc.LibVLC;
import org.videolan.libvlc.Media;

import java.io.File;
import java.io.FileDescriptor;
import java.io.FileInputStream;
import java.io.IOException;


public class AudioPlayerRecorder extends Handler {
    private static final String TAG = "AudioPlayerRecorder";

    private static MediaRecorder mRecorder = null;
    private static MediaPlayer   mPlayer = null;
    private static FileInputStream input = null;
    private static String currentFile = null;
    private static FileUtility.FileLocation _location;
    private static float volume = 1;
    private static String[] audioTypes = {"mp3", "3gp", "flac", "ogg", "wav"};


    private static LibVLC libVLC = null;
    private static org.videolan.libvlc.MediaPlayer vlcMediaPlayer = null;

    private static boolean useVLC = false;
    private static float desiredPlaybackRate; //Playback rate must be kept between sessions (when restarting video)
    
    public static native void notifyPlayingSoundEnded();
    private static AudioPlayerRecorder instance = null;
    public static AudioPlayerRecorder getInstance()
    {
        if(instance == null)
        {
            instance = new AudioPlayerRecorder();
        }
        return instance;
    }

    @SuppressWarnings("unused")
    public static void setUseVLC(boolean use)
    {
        if(mPlayer != null)
        {
            Log.e(TAG, "Can't change AudioPlayer VLC/MediaPlayer mode after starting it");
            return;
        }
        if(use && !useVLC)
        { //If the app crash here, check that libvlc is properly compiled using compile.sh
            try
            {
                libVLC = new LibVLC();
                Log.i(TAG, "LibVLC loaded");
            }
            catch(Exception e)
            {
                Log.e(TAG, "Exception while loading vlcjni, it's probably not compiled for the current architecture. Falling back to MediaPlayer");
                e.printStackTrace();
                useVLC = false;
            }
        }
        useVLC = use;
    }

    @SuppressWarnings("WeakerAccess")
    public static boolean isPlaying()
    {
    	Log.d(TAG, "returning isPlaying");
        if(useVLC) {
            return vlcMediaPlayer != null && vlcMediaPlayer.isPlaying();
        }
        else {
            return mPlayer != null && mPlayer.isPlaying();
        }
    }

    @SuppressWarnings("unused")
    public static boolean isRecording()
    {
    	Log.d(TAG, "returning isRecording");
    	return mRecorder != null;
    }

    @SuppressWarnings("unused")
    public static void startPlaying(String fileName, float newVolume)
    {
        if(useVLC) {
            currentFile = startVLCPlayer(libVLC, fileName, true, newVolume);
        }
        else {
            mPlayer = new MediaPlayer();
            mPlayer.setOnErrorListener(new OnErrorListener() {
                public boolean onError(MediaPlayer mp, int what, int extra) {
                    Log.e(TAG, "Error with MediaPlayer, stopping it. What : "
                            + (what == MediaPlayer.MEDIA_ERROR_UNKNOWN ? "Unknown" : "Server died")
                            + ", extra : "
                            + (extra == MediaPlayer.MEDIA_ERROR_IO ? "IO Error" :
                            extra == MediaPlayer.MEDIA_ERROR_UNSUPPORTED ? "Unsupported" :
                                    extra == MediaPlayer.MEDIA_ERROR_MALFORMED ? "Malformed" :
                                            "Timed out"));
                    AudioPlayerRecorder.stopPlaying();
                    return true;
                }
            });
            mPlayer.setOnPreparedListener(new OnPreparedListener() {
                public void onPrepared(MediaPlayer mp) {
                    mp.start();
                    Log.i(TAG, "MediaPlayer started");
                }
            });
            mPlayer.setOnCompletionListener(new OnCompletionListener() {
                public void onCompletion(MediaPlayer mp) {
                    notifyPlayingSoundEnded();
                }
            });
            currentFile = startMediaPlayer(mPlayer, fileName, true, true, newVolume);
        }
        if(currentFile == null)
        {
        	mPlayer = null;
        	input = null;
        }
        volume = newVolume;
    }

    @SuppressWarnings("unused")
    public static void playIndependent(String fileName, float newVolume)
    {
        MediaPlayer tmpPlayer = new MediaPlayer();
        tmpPlayer.setOnErrorListener(new OnErrorListener() {
            public boolean onError(MediaPlayer mp, int what, int extra) {
                Log.e(TAG, "Error with MediaPlayer (getSoundDuration), stopping it. What : "
                        + (what == MediaPlayer.MEDIA_ERROR_UNKNOWN ? "Unknown" : "Server died")
                        + ", extra : "
                        + (extra == MediaPlayer.MEDIA_ERROR_IO ? "IO Error" :
                        extra == MediaPlayer.MEDIA_ERROR_UNSUPPORTED ? "Unsupported" :
                                extra == MediaPlayer.MEDIA_ERROR_MALFORMED ? "Malformed" :
                                        "Timed out"));
                AudioPlayerRecorder.stopPlaying();
                return true;
            }
        });
        tmpPlayer.setOnPreparedListener(new OnPreparedListener() {
            public void onPrepared(MediaPlayer mp) {
                mp.start();
                Log.i(TAG, "Independent MediaPlayer started");
            }
        });
        tmpPlayer.setOnCompletionListener(new OnCompletionListener() {
            public void onCompletion(MediaPlayer mp) {
                mp.reset();
                mp.release();
                Log.i(TAG, "Independent MediaPlayer finished");
            }
        });
        startMediaPlayer(tmpPlayer, fileName, true, false, newVolume);
    }
    
    //Return the full name of the file
    private static String startMediaPlayer(MediaPlayer player, String file, boolean asyncPrepare, boolean isMain, float newVolume)
    {
        player.setVolume(newVolume, newVolume);
    	//try to load from data
    	String relativeName = file;
    	if(getFileExtension(file) == null) relativeName += ".mp3";
    	String fullName = FileUtility.findFullPath(relativeName);
    	
    	Log.d(TAG, "start MediaPlayer with path: " + fullName);
        File target = new File(fullName);
        //Try to load from full path
        if(target.exists())
        {
            try 
            {
            	FileInputStream stream = new FileInputStream(target);
            	if(isMain)
            		input = stream;
            	player.setDataSource(stream.getFD());
            	
            	if(asyncPrepare)
            		player.prepareAsync();
            	else
            		player.prepare();
            } 
            catch (IOException e) 
            {
            	if(isMain)
            	{
                	fullName = null;
            	}
                Log.e(TAG, "prepare() from full path failed, exception : " + e.getLocalizedMessage());
                notifyPlayingSoundEnded();
            }
        }
        //try to load from package using assets
        else
        {
            AssetManager am = NativeUtility.getMainActivity().getAssets();
        	Log.d(TAG, "start MediaPlayer from resources : " + relativeName);
            try 
            {
                AssetFileDescriptor descriptor = am.openFd(relativeName);
                FileDescriptor fileDesc = descriptor.getFileDescriptor();
                if(fileDesc.valid())
                {
                	player.setDataSource(fileDesc, descriptor.getStartOffset(), descriptor.getLength());
                	descriptor.close();
                	if(asyncPrepare)
                		player.prepareAsync();
                	else
                		player.prepare();
                }
                else
                {
                    Log.e(TAG, "No sound file matching : " + currentFile);
                	fullName = null;
                }
            } 
            catch (IOException e) 
            {
            	fullName = null;
                Log.e(TAG, "prepare() failed from resources, exception : " + e.getLocalizedMessage());
                notifyPlayingSoundEnded();
            }
        }
        return fullName;
    }


    private static class StartVLCRunnable implements Runnable
    {
        private LibVLC player;
        private String fullName;
        private boolean isMain;
        private float volume;
        private StartVLCRunnable(LibVLC _player, String _fullName, boolean _isMain, float _volume) {
            this.player = _player;
            this.fullName = _fullName;
            this.isMain = _isMain;
            this.volume = _volume;
        }

        public void run() {
            try
            {
                File target = new File(fullName);
                FileInputStream stream = new FileInputStream(target);
                if(isMain) input = stream;
                if(vlcMediaPlayer == null)
                {
                    vlcMediaPlayer = new org.videolan.libvlc.MediaPlayer(player);
                }
                Media m = new Media(player, fullName);
                //Native crash for no good reason, as if the instance is invalid (it has been init by LibVLC before)
                EventHandler.getInstance().addHandler(getInstance());
                vlcMediaPlayer.setMedia(m);
                vlcMediaPlayer.setVolume((int)(volume*100));
                vlcMediaPlayer.play();
                setPlaybackRate(desiredPlaybackRate);
                vlcMediaPlayer.setEventListener(new org.videolan.libvlc.MediaPlayer.EventListener() {
                    @Override
                    public void onEvent(org.videolan.libvlc.MediaPlayer.Event event) {
                        if(event.type == org.videolan.libvlc.MediaPlayer.Event.EndReached)
                        {
                            notifyPlayingSoundEnded();
                        }
                    }
                });

            }
            catch (IOException e)
            {
                if(isMain)
                {
                    fullName = null;
                }
                Log.e(TAG, "prepare() from full path failed, exception : " + e.getLocalizedMessage());
            }
        }
    }

    private static String startVLCPlayer(LibVLC player, String file, boolean isMain, float newVolume)
    {
        //try to load from data
        String relativeName = file;
        if(getFileExtension(file) == null) relativeName += ".mp3";
        String fullName = FileUtility.findFullPath(relativeName);

        //Use Environment.getExternalStorageDirectory().toString() ?
        Log.d(TAG, "start VLC Player with path: " + fullName);
        File target = new File(fullName);
        //Try to load from full path path
        if(target.exists())
        {
            //We have to implement a runnable to pass data, and because EventHandler can only be accessed on UI Thread
            StartVLCRunnable runnable = new StartVLCRunnable(player, fullName, isMain, newVolume);
            NativeUtility.getMainActivity().runOnUiThread(runnable);
        }
        //try to load from package using assets
        else
        {
            AssetManager am = NativeUtility.getMainActivity().getAssets();
            Log.d(TAG, "start VLC Player from resources : " + relativeName);
            try
            {
                //VLC can't read directly from package. Copy the file to files and read it from there
                AssetFileDescriptor descriptor = am.openFd(relativeName);
                FileDescriptor fileDesc = descriptor.getFileDescriptor();
                if(fileDesc.valid())
                {
                    NativeUtility.copyResourceFileToLocal(relativeName);
                    startVLCPlayer(player, relativeName, isMain, newVolume);
                }
                else
                {
                    Log.e(TAG, "No sound file matching : " + currentFile);
                    fullName = null;
                }
            }
            catch (IOException e)
            {
                fullName = null;
                Log.e(TAG, "prepare() failed from resources, exception : " + e.getLocalizedMessage());
            }
        }
        return fullName;
    }

    @SuppressWarnings("WeakerAccess")
    public static void stopPlaying()
    {
        Log.d(TAG, "stop playing");
        if(useVLC)
        {
            if(vlcMediaPlayer != null) {
                vlcMediaPlayer.stop();
            }
            else
            {
                Log.e(TAG, "stop vlcMediaPlayer is Null");
            }
        }
        else if(mPlayer != null)
    	{
            if(mPlayer.isPlaying())
            {
                mPlayer.stop();
            }

            mPlayer.release();
            if(input != null)
            {
	            try {
	    			input.close();
	    		} catch (IOException e) {
	    			// TODO Auto-generated catch block
	    			e.printStackTrace();
	    		}
	            input = null;
            }
            mPlayer = null;
    	}
    }

    @SuppressWarnings("unused")
    public static void startRecording(String fileName, int location)
    {
    	currentFile = fileName;
        _location = FileUtility.FileLocation.valueOf(location);
        String fullPath = FileUtility.getFullPath(currentFile, _location);
    	Log.d(TAG, "start recording : " + fullPath);
        mRecorder = new MediaRecorder();
        mRecorder.setAudioSource(MediaRecorder.AudioSource.MIC);
        mRecorder.setOutputFormat(MediaRecorder.OutputFormat.THREE_GPP);
        mRecorder.setAudioChannels(1);
        mRecorder.setAudioSamplingRate(44100);
        mRecorder.setOutputFile(fullPath);
        mRecorder.setAudioEncoder(MediaRecorder.AudioEncoder.AMR_NB);

        try 
        {
            mRecorder.prepare();
            mRecorder.start();
        } 
        catch (IOException e) 
        {
            Log.e(TAG, "prepare() failed");
        }
        catch(IllegalStateException e)
        {
            Log.e(TAG, "start() failed, recorder wasn't ready");
        }

    }

    @SuppressWarnings("unused")
    public static void stopRecording() 
    {
        Log.d(TAG, "stop recording");
        if(mRecorder != null) {
            try{
                mRecorder.stop();
            }catch(RuntimeException stopException){
                Log.d(TAG, "stop RuntimeException, nothing got recorded");
            }
            mRecorder.release();
            mRecorder = null;
        }
    }

    @SuppressWarnings("unused")
    public static void fadeVolumeOut()
    {
    	final float speed = 0.010f;

        Log.d(TAG, "fade volume out");
        if(useVLC)
        {
            new Thread(new Runnable() {
                public void run() {
                    if(vlcMediaPlayer != null)
                    {
                        int originalVolume = vlcMediaPlayer.getVolume();
                        while(volume >= 0 && vlcMediaPlayer.isPlaying())
                        {
                            volume -= speed;
                            vlcMediaPlayer.setVolume((int)(volume * originalVolume));
                            try {
                                Thread.sleep(25);
                            } catch (InterruptedException e) {
                                e.printStackTrace();
                            }
                        }
                        vlcMediaPlayer.stop();
                        vlcMediaPlayer.setVolume(originalVolume);
                    }
                    else
                    {
                        Log.e(TAG, "fadeVolumeOut vlcMediaPlayer is Null");
                    }
                }
            }).start();
        }
    	else if(mPlayer != null)
    	{
    		new Thread(new Runnable() {
    	        public void run() {
                  try
                    {
                        while(mPlayer != null && volume >= 0 && mPlayer.isPlaying())
                        {
                            volume -= speed;
                            mPlayer.setVolume(volume, volume);
                            try {
                                Thread.sleep(25);
                            } catch (InterruptedException e) {
                                e.printStackTrace();
                            }
                        }
                        if(mPlayer != null && mPlayer.isPlaying())
                        {
                            mPlayer.stop();
                        }
                        volume = 1;
                    }
                    catch(java.lang.IllegalStateException e)
                    {
                        Log.e(TAG, "mPlayer isn't initialized yet");
                    }
    	        }
    	    }).start();
    	}
    }

    @SuppressWarnings("unused")
    public static float getPlaybackRate()
    {
        if(useVLC)
        {
            return desiredPlaybackRate;
        }
        Log.e(TAG, "getPlaybackRate is only implemented for LibVLC");
        return 1;
    }

    @SuppressWarnings("WeakerAccess")
    public static void setPlaybackRate(float rate)
    {
        if(useVLC)
        {
            if(vlcMediaPlayer != null) {
                vlcMediaPlayer.setRate(rate);
            }
            else
            {
                Log.e(TAG, "setPlaybackRate vlcMediaPlayer is Null");
            }
            desiredPlaybackRate = rate;
        }
        else
        {
            Log.e(TAG, "setPlaybackRate is only implemented for LibVLC");
        }
    }

    public static void play()
    {
    	Log.d(TAG, "play music");
        if(useVLC) {
            if(vlcMediaPlayer != null)
                vlcMediaPlayer.play();
        }
    	else if(mPlayer != null)
    			mPlayer.start();
    }

    public static void pause()
    {
    	Log.d(TAG, "pause music");
        if(useVLC) {
            if(vlcMediaPlayer != null)
            {
                vlcMediaPlayer.pause();
            }
            else
            {
                Log.e(TAG, "pause vlcMediaPlayer is Null");
            }
        }
    	else if(mPlayer != null)
    		if(mPlayer.isPlaying())
    			mPlayer.pause();
    }

    @SuppressWarnings("unused")
    public static void restart()
    {
        Log.d(TAG, "restart music");
    	//stopPlaying();
    	//startPlaying(file);

        if(useVLC) {
            if(vlcMediaPlayer != null) {
                vlcMediaPlayer.setTime(0);
                if(!vlcMediaPlayer.isPlaying())
                    vlcMediaPlayer.play();
            }
            else
            {
                Log.e(TAG, "restart vlcMediaPlayer is Null");
            }
        }
        else if(mPlayer != null) {
            mPlayer.seekTo(0);
            if(!isPlaying()) {
                play();
            }
        }
    }

    @SuppressWarnings("unused")
    public static float getSoundDuration(String fileName)
    {
        MediaPlayer tmpPlayer = new MediaPlayer();
        tmpPlayer.setOnErrorListener(new OnErrorListener() {
            public boolean onError(MediaPlayer mp, int what, int extra) {
                Log.e(TAG, "Error with MediaPlayer (getSoundDuration), stopping it. What : "
            + (what == MediaPlayer.MEDIA_ERROR_UNKNOWN ? "Unknown" : "Server died")
            + ", extra : "
            + (extra == MediaPlayer.MEDIA_ERROR_IO ? "IO Error" : 
            	extra == MediaPlayer.MEDIA_ERROR_UNSUPPORTED ? "Unsupported" : 
            		extra == MediaPlayer.MEDIA_ERROR_MALFORMED ? "Malformed" :
            			"Timed out"));
                AudioPlayerRecorder.stopPlaying();
                return true;
            }
        });
        
        String fullName = startMediaPlayer(tmpPlayer, fileName, false, false, 1);
        //Convert to seconds
        float duration = (float) (fullName != null ? (float)tmpPlayer.getDuration() / 1000.0 : 0.0);
        tmpPlayer.reset();
        tmpPlayer.release();
        return duration;
    }
    
    //Will return the '.' with the extension
    @SuppressWarnings("WeakerAccess")
    public static String getFileExtension(String file)
    {
    	if(file.contains("."))
    	{
    		String extension = file.substring(file.lastIndexOf('.'));
    		if(isAValidAudioFormat(extension))
    			return file.substring(file.indexOf('.'));
    	}
    		return null;
    }

    @SuppressWarnings("WeakerAccess")
    public static boolean isAValidAudioFormat(String extension)
    {
        for (String audioType : audioTypes) {
            if (extension.endsWith(audioType))
                return true;
        }
    	return false;
    }

    @SuppressWarnings("unused")
    public static boolean checkMicrophonePermission ()
    {
    	boolean permissionOK = NativeUtility.getMainActivity().checkCallingOrSelfPermission("android.permission.RECORD_AUDIO") == PackageManager.PERMISSION_GRANTED;
    	if(!permissionOK)
    	{
    		Toast.makeText(NativeUtility.getMainActivity(), "Warning, missiong RECORD_AUDIO permission", Toast.LENGTH_LONG).show();
    	}
    	return permissionOK;
    }
    
    private static MediaMetadataRetriever getMetadata(String file)
    {
    	String relativeName = file;
    	if(getFileExtension(file) == null) relativeName += ".mp3";
    	String fullName = FileUtility.findFullPath(relativeName);
    	
        File target = new File(fullName);
        //Try to load from full path path
        MediaMetadataRetriever retriever = new MediaMetadataRetriever();
        if(target.exists())
        {
            try 
            {
            	FileInputStream stream = new FileInputStream(target);
            	retriever.setDataSource(stream.getFD());
            } 
            catch (IOException e) 
            {
                Log.e(TAG, "MediaMetadataRetriever from full path failed, exception : " + e.getLocalizedMessage());
            }
        }
        //try to load from package using assets
        else
        {
            AssetManager am = NativeUtility.getMainActivity().getAssets();
            try 
            {
                AssetFileDescriptor descriptor = am.openFd(relativeName);
                FileDescriptor fileDesc = descriptor.getFileDescriptor();
                if(fileDesc.valid())
                {
                	retriever.setDataSource(fileDesc, descriptor.getStartOffset(), descriptor.getLength());
                	descriptor.close();
                }
                else
                {
                    Log.e(TAG, "No sound file matching : " + currentFile);
                }
            } 
            catch (IOException e) 
            {
                Log.e(TAG, "MediaMetadataRetriever failed from resources, exception : " + e.getLocalizedMessage());
            }
        }
        return retriever;
    }

    @SuppressWarnings("unused")
    public static String getAuthor(String file)
    {
        return getMetadata(file).extractMetadata(MediaMetadataRetriever.METADATA_KEY_ARTIST);
    }

    @SuppressWarnings("unused")
    public static String getTitle(String file)
    {
        return getMetadata(file).extractMetadata(MediaMetadataRetriever.METADATA_KEY_TITLE);
    }

    @Override
    public void handleMessage(Message msg)
    {
        int event = msg.getData().getInt("event", -1);
        if(event == EventHandler.MediaPlayerEncounteredError)
        {
            NativeUtility.getMainActivity().runOnGLThread(new Runnable()
            {
                public void run()
                {
                    notifyPlayingSoundEnded();
                }
            });
        }
        else if(event == EventHandler.MediaPlayerEndReached)
        {
            NativeUtility.getMainActivity().runOnGLThread(new Runnable()
            {
                public void run()
                {
                    notifyPlayingSoundEnded();
                }
            });
        }
        else if(event == EventHandler.HardwareAccelerationError)
        {
            Log.i(TAG, "Hardware Acceleration Error, disabling hardware acceleration");
        }
    }
}
