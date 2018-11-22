#!/system/bin/sh

do_reset_radio() {
  if test -f /tmp/.gsmreset.lock ; then
    return
  fi

  touch /tmp/.gsmreset.lock

  status=$(getprop gsm.status)
  # exit if in flight mode
  if [ "$status" == "2" ] ; then
    return
  fi

  subId=$(getprop gsm.subid)
  resetCount=$(getprop gsm.resetcount)
  if [ -z $resetCount ] ; then
    resetCount=0
  fi

  if [ "$subId" != "1" ] ; then
    sleep 10
  fi

  status=$(getprop gsm.status)
  if [ "$status" == "2" ] ; then
    return
  fi

  if [ "$subId" != "1" ] ; then
    stop ril-daemon
    sleep 5
    start ril-daemon
    setprop gsm.resetcount $(( $resetCount + 1 ))
  fi

  rm -f /tmp/.gsmreset.lock
  setprop gsm.radioreset false
}

do_reset_radio
