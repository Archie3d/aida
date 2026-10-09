procedure Imagecalls is
    type Fixed is delta 0.125 range -1000.0 .. 1000.0;
    Count : Integer := 0;
    procedure Consume (Text : String) is
    begin
        Count := Count + Text'Length;
    end Consume;
begin
    for I in 1 .. 200_000 loop
        Consume (Integer'Image (123));
        Consume (Fixed'Image (123.125));
    end loop;
    if Count /= 200_000 * (9 + Fixed'Aft) then
        raise Program_Error;
    end if;
end Imagecalls;
