#pragma once
#include "../../SDK/sdk.hpp"

namespace Interfaces
{

	class __declspec(novtable) AntiAimbot : public NonCopyable {
	public:
		static Encrypted_t<AntiAimbot> Get();
		virtual void Main(bool* bSendPacket, bool* bFinalPacket, Encrypted_t<CUserCmd> cmd, bool ragebot) = 0;
		virtual void PrePrediction(bool* bSendPacket, Encrypted_t<CUserCmd> cmd) = 0;
		virtual void ImposterBreaker(bool* bSendPacket, Encrypted_t<CUserCmd> cmd) = 0;

		float m_flLowerBodyUpdateYaw = FLT_MAX;
	protected:
		AntiAimbot() { };
		virtual ~AntiAimbot() { };
	};
}


/*#pragma once
#include "../../SDK/sdk.hpp"

namespace Interfaces
{

   class __declspec( novtable ) AntiAimbot : public NonCopyable {
   public:
	  static Encrypted_t<AntiAimbot> Get( );
	  virtual void Main( bool* bSendPacket, bool* bFinalPacket, Encrypted_t<CUserCmd> cmd, bool ragebot ) = 0;
	  virtual void PrePrediction( bool* bSendPacket, Encrypted_t<CUserCmd> cmd ) = 0;
	  virtual void ImposterBreaker( bool* bSendPacket, Encrypted_t<CUserCmd> cmd ) = 0;
   protected:
	  AntiAimbot( ) { };
	  virtual ~AntiAimbot( ) { };
   };
}
 */