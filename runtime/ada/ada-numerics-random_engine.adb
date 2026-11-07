package body Ada.Numerics.Random_Engine is
    procedure Encode (Data : System.Address; Text : out String);
    pragma Import (C, Encode, "__ada_random_image");
    procedure Decode (Text : String; Data : System.Address);
    pragma Import (C, Decode, "__ada_random_value");
    function Image_Of (Data : State_Words) return String is
        Result : String (1 .. Image_Width);
    begin
        Encode (Data'Address, Result);
        return Result;
    end Image_Of;
    function Value_Of (Text : String) return State_Words is
        Result : State_Words;
    begin
        Decode (Text, Result'Address);
        return Result;
    end Value_Of;
end Ada.Numerics.Random_Engine;
