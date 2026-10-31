procedure AggregateOverloadErrors is
    type First_Record is record
        Value : Integer;
    end record;
    type Second_Record is record
        Other : Integer;
    end record;
    procedure Pick (Value : First_Record) is null;
    procedure Pick (Value : Second_Record) is null;
begin
    -- Component names do not disambiguate two visible record types.
    Pick ((Value => 1));
end AggregateOverloadErrors;
