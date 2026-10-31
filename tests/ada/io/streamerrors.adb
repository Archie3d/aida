with Ada.Streams; use Ada.Streams;
procedure StreamErrors is
    type Value is new Integer;
    procedure Wrong_Read (Stream : access Root_Stream_Type'Class; Item : in Value);
    for Value'Read use Wrong_Read;
    type Small is range 0 .. 10;
    for Small'Stream_Size use 7;
    subtype Sub is Small;
    for Sub'Stream_Size use 16;
    type Frozen is new Integer;
    Item : Frozen;
    procedure Write_Frozen (Stream : access Root_Stream_Type'Class; Item : Frozen);
    for Frozen'Write use Write_Frozen;
    type Abstract_Item is abstract tagged null record;
    type Limited_Item is limited null record;
    Channel : access Root_Stream_Type'Class;
    Plain : access Integer;
    Limited_Object : Limited_Item;
    procedure Wrong_Read (Stream : access Root_Stream_Type'Class; Item : in Value) is
    begin null; end Wrong_Read;
    procedure Write_Frozen (Stream : access Root_Stream_Type'Class; Item : Frozen) is
    begin null; end Write_Frozen;
begin
    Integer'Write (Plain, 1);
    Limited_Item'Write (Channel, Limited_Object);
    declare
        Bad : Abstract_Item'Class := Abstract_Item'Input (Channel);
    begin null; end;
end StreamErrors;
