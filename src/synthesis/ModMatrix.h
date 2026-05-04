#ifndef MOD_MATRIX_H
#define MOD_MATRIX_H

#include <atomic>

enum class ModSource {
    None = 0,
    ADSR,
    LFO,
    Morph,
    Velocity,
    Count
};

enum class ModDestination {
    None = 0,
    FilterCutoff,
    OscFrequency,
    OscShape,
    VCAGain,
    ReverbMix,
    CrystalsPitch,
    Count
};

struct ModulationSlot {
    std::atomic<int> source{0};
    std::atomic<int> destination{0};
    std::atomic<double> amount{0.0};
    std::atomic<bool> enabled{true};
};

class ModulationMatrix {
public:
    static constexpr int NumSlots = 8; // Support 8 dynamic routings

    ModulationMatrix() {
        for (int i = 0; i < NumSlots; ++i) {
            m_slots[i].source.store(0);
            m_slots[i].destination.store(0);
            m_slots[i].amount.store(0.0);
        }
    }

    void setSlot(int index, int src, int dest, double amount) {
        if (index < 0 || index >= NumSlots) return;
        m_slots[index].source.store(src);
        m_slots[index].destination.store(dest);
        m_slots[index].amount.store(amount);
    }

    void setSlot(int index, int src, int dest, double amount, bool enabled) {
        if (index < 0 || index >= NumSlots) return;
        m_slots[index].source.store(src);
        m_slots[index].destination.store(dest);
        m_slots[index].amount.store(amount);
        m_slots[index].enabled.store(enabled);
    }

    int getSource(int index) const { return m_slots[index].source.load(); }
    int getDestination(int index) const { return m_slots[index].destination.load(); }
    double getAmount(int index) const { return m_slots[index].amount.load(); }
    bool isEnabled(int index) const { return m_slots[index].enabled.load(); }

    void setEnabled(int index, bool enabled) {
        if (index < 0 || index >= NumSlots) return;
        m_slots[index].enabled.store(enabled);
    }

    void clearAll() {
        for (int i = 0; i < NumSlots; ++i) {
            m_slots[i].source.store(0);
            m_slots[i].destination.store(0);
            m_slots[i].amount.store(0.0);
            m_slots[i].enabled.store(true); // Default to enabled
        }
    }

private:
    ModulationSlot m_slots[NumSlots];
};

#endif // MOD_MATRIX_H