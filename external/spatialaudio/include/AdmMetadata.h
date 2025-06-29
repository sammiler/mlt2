/*############################################################################*/
/*#                                                                          #*/
/*#  ADM metadata struct SA_APIures.	                                             #*/
/*#								                                             #*/
/*#  Copyright © 2020 Peter Stitt                                            #*/
/*#                                                                          #*/
/*#  Filename:      AdmMetadata.h                                            #*/
/*#  Version:       0.1                                                      #*/
/*#  Date:          23/06/2020                                               #*/
/*#  Author(s):     Peter Stitt                                              #*/
/*#  Licence:       LGPL + proprietary                                       #*/
/*#                                                                          #*/
/*############################################################################*/

#pragma once

#include <vector>
#include <string>
#include <map>
#include "Coordinates.h"
#include "ScreenCommon.h"
#include "sa_api.h"
namespace admrender {

	// The different audio types expected from the ADM metadata
	// Rec. ITU-R BS.2127-0 pg. 8
	enum class TypeDefinition {
		DirectSpeakers = 1,
		Matrix,
		Objects,
		HOA,
		Binaural
	};

	// Shared struct SA_APIure (Rec.ITU - R BS.2127 - 0 section 11.1.1) ============================================================
	// Frequency data for the channel
	struct SA_API Frequency {
		std::vector<double> lowPass;
		std::vector<double> highPass;
	};
	inline bool operator==(const Frequency& lhs, const Frequency& rhs)
	{
		return lhs.lowPass == rhs.lowPass && lhs.highPass == rhs.highPass;
	}

	struct SA_API ChannelLock
	{
		// If the distance is set <0 then no channel locking is applied
		double maxDistance = -1.;
	};
	inline bool operator==(const ChannelLock& lhs, const ChannelLock& rhs)
	{
		return lhs.maxDistance == rhs.maxDistance;
	}
	struct SA_API ObjectDivergence
	{
		double value = 0.;
		double azimuthRange = 180.;
		double positionRange = 1.;
	};
	inline bool operator==(const ObjectDivergence& lhs, const ObjectDivergence& rhs)
	{
		return lhs.value == rhs.value && lhs.azimuthRange == rhs.azimuthRange && lhs.positionRange == rhs.positionRange;
	}

	struct SA_API ScreenEdgeLock {
		enum Horizontal { NO_HOR, LEFT, RIGHT };
		enum Vertical { NO_VERT, BOTTOM, TOP };
		Horizontal horizontal = NO_HOR;
		Vertical vertical = NO_VERT;
	};
	inline bool operator==(const ScreenEdgeLock& lhs, const ScreenEdgeLock& rhs)
	{
		return lhs.horizontal == rhs.horizontal && lhs.vertical == rhs.vertical;
	}

	// struct SA_APIures to hold positions of objects
	struct SA_API PolarBounds 
	{
		double minAzimuth;
		double maxAzimuth;
		double minElevation;
		double maxElevation;
		double minDistance;
		double maxDistance;
	};
	inline bool operator==(const PolarBounds& lhs, const PolarBounds& rhs)
	{
		return lhs.minAzimuth == rhs.minAzimuth && lhs.maxAzimuth == rhs.maxAzimuth
			&& lhs.minElevation == rhs.minElevation && lhs.maxElevation == rhs.maxElevation
			&& lhs.minDistance == rhs.minDistance && lhs.maxDistance == rhs.maxDistance;
	}
	struct SA_API CartesianBounds
	{
		double minX;
		double maxX;
		double minY;
		double maxY;
		double minZ;
		double maxZ;
	};
	inline bool operator==(const CartesianBounds& lhs, const CartesianBounds& rhs)
	{
		return lhs.minX == rhs.minX && lhs.maxX == rhs.maxX
			&& lhs.minY == rhs.minY && lhs.maxY == rhs.maxY
			&& lhs.minZ == rhs.minZ && lhs.maxZ == rhs.maxZ;
	}

	struct SA_API JumpPosition
	{
		bool flag = false;
		// This is the duration of the interpolation in samples
		int interpolationLength = 0;
	};
	inline bool operator==(const JumpPosition& lhs, const JumpPosition& rhs)
	{
		return lhs.flag == rhs.flag && lhs.interpolationLength == rhs.interpolationLength;
	}

	struct SA_API DirectSpeakerPolarPosition
	{
		double azimuth = 0.0;
		double elevation = 0.0;
		double distance = 1.f;
		// Bounds for speaker used in DirectSpeaker gain calculation
		std::vector<PolarBounds> bounds;
	};
	inline bool operator==(const DirectSpeakerPolarPosition& lhs, const DirectSpeakerPolarPosition& rhs)
	{
		return lhs.azimuth == rhs.azimuth && lhs.elevation == rhs.elevation
			&& lhs.distance == rhs.distance && lhs.bounds == rhs.bounds;
	}
	struct SA_API DirectSpeakerCartesianPosition
	{
		double x = 1.0;
		double y = 0.0;
		double z = 0.0;
		// Bounds for speaker used in DirectSpeaker gain calculation
		std::vector<CartesianBounds> bounds;
	};
	inline bool operator==(const DirectSpeakerCartesianPosition& lhs, const DirectSpeakerCartesianPosition& rhs)
	{
		return lhs.x == rhs.x && lhs.y == rhs.y
			&& lhs.z == rhs.z && lhs.bounds == rhs.bounds;
	}

	struct SA_API ExclusionZone { };

	struct SA_API CartesianExclusionZone : ExclusionZone {
		float minX;
		float minY;
		float minZ;
		float maxX;
		float maxY;
		float maxZ;
	};
	inline bool operator==(const CartesianExclusionZone& lhs, const CartesianExclusionZone& rhs)
	{
		return lhs.minX == rhs.minX && lhs.maxX == rhs.maxX
			&& lhs.minY == rhs.minY && lhs.maxY == rhs.maxY
			&& lhs.minZ == rhs.minZ && lhs.maxZ == rhs.maxZ;
	}

	struct SA_API PolarExclusionZone : ExclusionZone {
		float minElevation;
		float maxElevation;
		float minAzimuth;
		float maxAzimuth;
	};
	inline bool operator==(const PolarExclusionZone& lhs, const PolarExclusionZone& rhs)
	{
		return lhs.minAzimuth == rhs.minAzimuth && lhs.maxAzimuth == rhs.maxAzimuth
			&& lhs.minElevation == rhs.minElevation && lhs.maxElevation == rhs.maxElevation;
	}

	// Metadata for different objects. See Rec. ITU-R BS.2127-0 page 86.

	// The metadata for ObjectType
	struct SA_API ObjectMetadata
	{
		PolarPosition polarPosition;
		CartesianPosition cartesianPosition;
		// Gain of the Object metadata
		double gain = 1.0;
		// Diffuseness parameter
		double diffuse = 0.0;
		// Channel lock distance. values < 0 mean no processing is applied
		ChannelLock channelLock;
		// Object divergence parameters
		ObjectDivergence objectDivergence;
		// Flag if cartesian position coordinates
		bool cartesian = false;
		// Extent parameters
		double width = 0.0;
		double height = 0.0;
		double depth = 0.0;
		// Jump position to determine how the gains are interpolated
		JumpPosition jumpPosition;
		// The track index (starting from 0)
		unsigned int trackInd = 0;
		std::vector<PolarExclusionZone> zoneExclusionPolar;
		// Screen reference for screen scaling
		bool screenRef = false;
		// Screen lock
		ScreenEdgeLock screenEdgeLock;
		// The length of the block in samples
		unsigned int blockLength = 0;
		// The reference screen
		std::vector<Screen> referenceScreen;
	};
	inline bool operator==(const ObjectMetadata& lhs, const ObjectMetadata& rhs)
	{
		return lhs.polarPosition == rhs.polarPosition && lhs.cartesianPosition == rhs.cartesianPosition
			&& lhs.gain == rhs.gain && lhs.diffuse == rhs.diffuse
			&& lhs.channelLock == rhs.channelLock && lhs.objectDivergence == rhs.objectDivergence
			&& lhs.width == rhs.width && lhs.height == rhs.height && lhs.depth == rhs.depth
			&& lhs.cartesian == rhs.cartesian && lhs.jumpPosition == rhs.jumpPosition
			&& lhs.trackInd == rhs.trackInd && lhs.zoneExclusionPolar == rhs.zoneExclusionPolar
			&& lhs.screenEdgeLock == rhs.screenEdgeLock && lhs.screenRef == rhs.screenRef
			&& lhs.blockLength == rhs.blockLength;
	}

	// The metadata for HoaType
	struct SA_API HoaMetadata
	{
		// A vector containing the HOA orders of each of the channels
		std::vector<int> orders;
		// The degree of each channel where -order <= degree <= +order
		std::vector<int> degrees;
		// The normalization scheme of the HOA signal
		std::string normalization = "SN3D";
		std::vector<unsigned int> trackInds;
	};
	inline bool operator==(const HoaMetadata& lhs, const HoaMetadata& rhs)
	{
		return lhs.orders == rhs.orders && lhs.degrees == rhs.degrees
			&& lhs.normalization == rhs.normalization && lhs.trackInds == rhs.trackInds;
	}
	// The metadata for DirectSpeaker
	// See See Rec. ITU-R BS.2127-0 page 63.
	struct SA_API DirectSpeakerMetadata
	{
		// The speaker labels from the stream metadata
		std::string speakerLabel = {};
		// The position of the loudspeaker
		DirectSpeakerPolarPosition polarPosition;
		// The track index (starting from 0)
		unsigned int trackInd = 0;
		// audioPackFormatID
		std::vector<std::string> audioPackFormatID;
		// Channel frequency information
		Frequency channelFrequency;
		// Screen edge lock
		ScreenEdgeLock screenEdgeLock;
	};
	inline bool operator==(const DirectSpeakerMetadata& lhs, const DirectSpeakerMetadata& rhs)
	{
		return lhs.speakerLabel == rhs.speakerLabel && lhs.polarPosition == rhs.polarPosition
			&& lhs.trackInd == rhs.trackInd && lhs.audioPackFormatID == rhs.audioPackFormatID
			&& lhs.channelFrequency == rhs.channelFrequency;
	}

	// Information about all of the channels in the stream. Contains the type of each track
	// and the number of channels
	struct SA_API StreamInformation
	{
		std::vector<TypeDefinition> typeDefinition;
		unsigned int nChannels;
	};

	// Rec. ITU-R BS.2127-0 Table 15
	const std::map<std::string, std::string> ituPackNames = {{"AP_00010001", "0+1+0"},
	{"AP_00010002", "0+2+0"},
	{"AP_0001000c", "0+5+0"},
	{"AP_00010003", "0+5+0"},
	{"AP_00010004", "2+5+0"},
	{"AP_00010005", "4+5+0"},
	{"AP_00010010", "4+5+1"},
	{"AP_00010007", "3+7+0"},
	{"AP_00010008", "4+9+0"},
	{"AP_00010009", "9+10+3"},
	{"AP_0001000f", "0+7+0"},
	{"AP_00010017", "4+7+0"},
	};

}
