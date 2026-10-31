package body GenericAbstractModel is
    function Read (Hidden : Child; Extra : Offset := 2) return Integer is
    begin
        return Hidden.Value + Extra;
    end Read;
    procedure Change (Hidden : in out Child; Amount : Integer := 2) is
    begin
        Hidden.Value := Hidden.Value + Amount;
    end Change;
    function Make (Value : Integer := 3) return Child is
    begin
        return (Value => Value);
    end Make;
    function Pointer_Read (Object : access Child) return Integer is
    begin
        return Object.Value + 10;
    end Pointer_Read;
end GenericAbstractModel;
