#ifndef RA_SERVICE_LOCATOR_HH
#define RA_SERVICE_LOCATOR_HH
#pragma once

#include "RA_Log.h"

#include <assert.h>
#include <memory>

namespace ra {
namespace services {

class ServiceLocator
{
public:
    /// <summary>
    /// Returns a reference to a service implementing the requested interface.
    /// </summary>
    /// <returns>Reference to an implementation of the interface</returns>
    /// <exception cref="std::runtime_error">No service is provided for the requested interface.</exception>
    template <class TClass>
    static const TClass& Get()
    {
        return Service<TClass>::Get();
    }

    /// <summary>
    /// Returns a non-const reference to a service implementing the requested interface.
    /// </summary>
    /// <returns>Reference to an implementation of the interface</returns>
    /// <exception cref="std::runtime_error">No service is provided for the requested interface.</exception>
    /// <remarks>
    /// WARNING: mutable services are not guaranteed to be thread-safe. Prefer using non-mutable 
    /// services whenever possible.
    /// </remarks>
    template <class TClass>
    static TClass& GetMutable()
    {
        return Service<TClass>::Get();
    }
    
    /// <summary>
    /// Registers a service implementation for an interface
    /// </summary>
    /// <param name="pInstance">
    /// A pointer to an allocated instance of a service implementation. ServiceLocator will own the 
    /// pointer and delete it at the end of execution or when a different implementation is provided.
    /// </param>
    template <class TClass>
    static void Provide(TClass* pInstance)
    {
        Service<TClass>::s_pInstance.reset(pInstance);
    }
    
    /// <summary>
    /// Provides a temporary implementation of an interface for the duration of the scope of the ServiceOverride.
    /// The original implementation will be restored when the <see cref="ServiceOverride"/> goes out of scope.
    /// </summary>
    /// <remarks>
    /// Primarily used in unit tests.
    /// </remarks>
    template <class TClass>
    class ServiceOverride
    {
    public:
        /// <summary>
        /// Initializes a new instance of the <see cref="ServiceOverride"/> class.
        /// </summary>
        /// <param name="pOverride">A pointer to the temporary implementation.</param>
        /// <param name="bDestroy">
        /// If set to <c>true</c>, the pointer will be owned by the <see cref="ServiceOverride"/> and deleted 
        /// when the <see cref="ServiceOverride"/> goes out of scope. This should normally be false, as 
        /// <paramref name="pOverride"/> will typically be a this pointer to the subclass, or the address of a 
        /// stack variable.
        /// </param>
        ServiceOverride(TClass* pOverride, bool bDestroy = false)
            : m_pPrevious(Service<TClass>::s_pInstance.release()), m_bDestroy(bDestroy)
        {
            Service<TClass>::s_pInstance.reset(pOverride);
        }

        ServiceOverride() = delete;

        ~ServiceOverride()
        {
            if (!m_bDestroy)
                Service<TClass>::s_pInstance.release();

            Service<TClass>::s_pInstance.reset(m_pPrevious);
        }
        
    private:
        TClass* m_pPrevious;
        bool m_bDestroy;
    };
    
private:
    template <class TClass>
    class Service
    {
    public:
        static TClass& Get() 
        {
            if (s_pInstance == nullptr)
                ThrowNoServiceProvidedException();

            return *s_pInstance.get();
        }
        
        static std::unique_ptr<TClass> s_pInstance;

    private:
        static void ThrowNoServiceProvidedException()
        {
#if defined(__PRETTY_FUNCTION__)
            std::string sMessage = __PRETTY_FUNCTION__;
#elif defined (__FUNCTION__)
            std::string sMessage = __FUNCTION__;
#else
            std::string sMessage = __FUNC__;
#endif
            size_t index = sMessage.find('<');
            size_t index2 = sMessage.rfind('>');
            if (index >= 0 && index2 > index)
            {
                sMessage.erase(index2);
                sMessage.erase(0, index + 1);
            }
            sMessage.insert(0, "No service provided for ");

            RA_LOG("ERROR: %s\n", sMessage.c_str());

            throw std::runtime_error(sMessage);
        }
    };
};

template <class TClass>
std::unique_ptr<TClass> ServiceLocator::Service<TClass>::s_pInstance;

} // namespace services
} // namespace ra

#endif // !RA_SERVICE_LOCATOR_HH
