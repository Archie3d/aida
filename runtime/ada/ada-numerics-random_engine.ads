with System;
package Ada.Numerics.Random_Engine is
    pragma Preelaborate;
    type Word is mod 2 ** 64;
    type State_Words is array (1 .. 4) of Word;
    -- SplitMix64 expansion of integer initiator zero.
    Default_State : constant State_Words :=
        (16#E220A8397B1DCDAF#, 16#6E789E6AA1B965F4#,
         16#06C45D188009454F#, 16#F88BB8A8724C81EC#);
    Image_Width : constant := 10 + 4 * 16;

    function Random_Float (Data : System.Address) return Float;
    pragma Import (C, Random_Float, "__ada_random_float");
    function Random_Discrete (Data : System.Address; First, Last : Word; Unsigned_Range : Boolean) return Word;
    pragma Import (C, Random_Discrete, "__ada_random_discrete");
    procedure Reset (Data : System.Address; Initiator : Integer);
    pragma Import (C, Reset, "__ada_random_seed");
    procedure Reset (Data : System.Address);
    pragma Import (C, Reset, "__ada_random_time");
    procedure Restore (Data, From : System.Address);
    pragma Import (C, Restore, "__ada_random_restore");
    function Image_Of (Data : State_Words) return String;
    function Value_Of (Text : String) return State_Words;
end Ada.Numerics.Random_Engine;
