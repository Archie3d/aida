-- The unit every stream is measured in.  A stream element is a byte, which is
-- what makes a stream a run of storage that any type can be spelled out into
-- and read back from.

package Ada.Streams is

    type Stream_Element is range 0 .. 255;
    for Stream_Element'Size use 8;

    type Stream_Element_Offset is range -2147483647 .. 2147483647;

    subtype Stream_Element_Count is Stream_Element_Offset range 0 .. 2147483647;

    type Stream_Element_Array is array (Stream_Element_Offset range <>) of aliased Stream_Element;

    type Root_Stream_Type is abstract tagged limited private;
    procedure Read (Stream : in out Root_Stream_Type;
                    Item : out Stream_Element_Array;
                    Last : out Stream_Element_Offset) is abstract;
    procedure Write (Stream : in out Root_Stream_Type;
                     Item : Stream_Element_Array) is abstract;
private
    type Root_Stream_Type is abstract tagged limited null record;

end Ada.Streams;
