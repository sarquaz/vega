vega is the execution enviroment with lua as a scripting language. All IO in vega will be asynchronous (implemented on the low level by [tau library](https://github.com/therealaquarius/tau)), but lua code will be synchronous (thanks to lua coroutines). 

vega is designed to be very effecient, to allow for beautiful code, and to be the basis for feature rich applications.
