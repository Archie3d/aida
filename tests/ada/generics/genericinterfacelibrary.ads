with GenericInterfaceModel;
generic
    type F is interface and GenericInterfaceModel.Base;
    type T is new F with private;
package GenericInterfaceLibrary is
    function Static_Read (X : T) return Integer;
    function Dynamic_Read (X : F'Class) return Integer;
end GenericInterfaceLibrary;
