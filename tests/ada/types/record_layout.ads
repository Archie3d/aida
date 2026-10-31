package Record_Layout is
    type Frame (Kind : Boolean := True; Inner : Boolean := False) is record
        Header : Integer := 9;
        case Kind is
            when True =>
                First : Integer := 11;
                Second : Integer := 22;
            when False =>
                case Inner is
                    when True => Letter : Character := 'q';
                    when False => Wide : Long_Integer := 33;
                end case;
        end case;
    end record;
    for Frame use record
        Kind at 12 range 0 .. 7;
        Inner at 13 range 0 .. 7;
        Header at 16 range 0 .. 31;
        First at 0 range 0 .. 31;
        Second at 4 range 0 .. 31;
        Letter at 0 range 0 .. 7;
        Wide at 0 range 0 .. 63;
    end record;
    for Frame'Size use 192;
    Global : Frame;
end Record_Layout;
