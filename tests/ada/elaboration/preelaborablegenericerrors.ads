generic
    type Element is private;
    Bound : Positive;
    with function Create return Integer;
package PreelaborableGenericErrors is
    pragma Preelaborate;
    procedure Dummy;
end PreelaborableGenericErrors;
