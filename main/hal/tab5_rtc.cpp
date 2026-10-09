#include "tab5_rtc.hpp"
#include <M5Unified.h>
#include <sys/time.h>

bool tab5_rtc_disponivel(void) { return M5.Rtc.isEnabled(); }

bool tab5_rtc_definir(time_t epoch_utc)
{
  if (!M5.Rtc.isEnabled()) { return false; }

  struct tm t = { };
  gmtime_r(&epoch_utc, &t);

  // RTC_Class::setDateTime(const tm*) - a sobrecarga mais direta, devolve
  // void (a classe trata falha de escrita por dentro, nao ha o que checar
  // aqui).
  M5.Rtc.setDateTime(&t);

  // O RELOGIO DO SISTEMA TAMBEM, na mesma chamada - ver o aviso no .hpp.
  struct timeval tv = { };
  tv.tv_sec  = epoch_utc;
  tv.tv_usec = 0;
  settimeofday(&tv, nullptr);

  return true;
}
