#ifndef VIS4EARTH_TF_CMPT_H
#define VIS4EARTH_TF_CMPT_H

#include <limits>
#include <memory>

#include <array>
#include <map>
#include <vector>

#include <QtCharts/QAreaSeries>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QScatterSeries>
#include <QtCore/QMetaEnum>
#include <QtGui/QKeyEvent>
#include <QtGui/QMouseEvent>
#include <QtWidgets/QApplication>
#include <QtWidgets/QColorDialog>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>

#include <vis4earth/data/tf_data.h>
#include <vis4earth/qt_util.h>

namespace VIS4Earth {

class TransferFunctionChartView : public QtCharts::QChartView {
    Q_OBJECT

  public:
    TransferFunctionChartView(QWidget *parent = nullptr) : QtCharts::QChartView(parent) {
        initSeries();

        chart()->setTheme(QtCharts::QChart::ChartThemeBlueCerulean);
        chart()->legend()->hide();
    }

    const TransferFunctionData &GetTransferFunctionData() const { return tfDat; }
    void SetTransferFunctionData(const TransferFunctionData &tfDat) {
        this->tfDat = tfDat;
        initSeries();
        updateSeriesFromData();
    }
    void SetFromGradientPreset(QGradient::Preset preset, float slopeAlpha = .33f) {
        QGradient gradient(preset);
        auto stops = gradient.stops();
        TransferFunctionData newTFDat;

        int i = 0;
        for (auto &stop : stops) {
            auto x = 1. * i / (stops.size() - 1);
            uint8_t scalar = std::round(255. * x);
            auto rgba = QColorToRGBA(stop.second);
            rgba[3] = slopeAlpha * x;

            newTFDat.ReplaceOrSetPoint(scalar, scalar, rgba);
            ++i;
        }
        assert(newTFDat.GetPoints().begin()->first == 0);
        assert(newTFDat.GetPoints().rbegin()->first == 255);

        this->tfDat = newTFDat;
        initSeries();
        updateSeriesFromData();
    }

    Optional<QColor> GetSelectedScatterColor() const {
        if (!selectedScatter)
            return {};

        auto color = selectedScatter->color();
        color.setAlpha(selectedScatter->at(0).y());
        return color;
    }
    void SetSelectedScatterColor(QColor color) {
        if (!selectedScatter)
            return;

        auto pnt = selectedScatter->at(0);
        pnt.setY(color.alpha());
        auto rgba = QColorToRGBA(color);
        color.setAlpha(255);
        selectedScatter->setColor(color);
        selectedScatter->insert(0, pnt);

        tfDat.ReplaceOrSetPoint(selectedScalar, selectedScalar, rgba);
        updateSeriesFromData();
        emit TransferFunctionChanged();
    }

  Q_SIGNALS:
    void ScatterUnselected();
    void ScatterSelectedOrChanged(const QPointF &point, const QColor &color);
    void TransferFunctionChanged();

  protected:
    void mousePressEvent(QMouseEvent *e) override {
        if (e->button() != Qt::MouseButton::LeftButton)
            return;

        unselect();

        auto posInChart = chart()->mapToValue(e->pos());
        auto distSqrMin = std::numeric_limits<float>::max();
        constexpr auto ErrSqr = 50.;
        for (auto &scalar_scatter : tfScatters) {
            auto dlt = posInChart - std::get<1>(scalar_scatter)->at(0);
            auto distSqr = QPointF::dotProduct(dlt, dlt);
            if (distSqr <= ErrSqr && distSqr < distSqrMin) {
                interactionScalar = std::get<0>(scalar_scatter);
                interactionScatter = std::get<1>(scalar_scatter);
                distSqrMin = distSqr;
            }
        }

        while (interactionKey == Qt::Key::Key_Control) {
            uint8_t newScalar = std::round(posInChart.x());
            if (interactionScatter && interactionScalar == newScalar)
                break; // don't add TF point when there is a selection

            interactionScalar = newScalar;
            interactionScatter = new QtCharts::QScatterSeries();
            posInChart.setX(newScalar);
            interactionScatter->append(posInChart);
            interactionScatter->setColor(Qt::GlobalColor::white);

            tfScatters.emplace(std::make_pair(interactionScalar, interactionScatter));
            chart()->addSeries(interactionScatter);
            updateLineFromScatterSeries();

            break;
        }

        if (interactionScatter) {
            interactionScatter->setMarkerShape(
                QtCharts::QScatterSeries::MarkerShape::MarkerShapeRectangle);

            selectFromInteraction();

            emit ScatterSelectedOrChanged(interactionScatter->at(0), interactionScatter->color());
        } else
            emit ScatterUnselected();
    }
    void mouseMoveEvent(QMouseEvent *e) override {
        if (!interactionScatter)
            return;

        auto posInChart = chart()->mapToValue(e->pos());
        interactionDir = posInChart.x() > interactionScatter->at(0).x() ? 1 : -1;

        if (posInChart.x() < 0.)
            posInChart.setX(0.);
        if (posInChart.x() > 255.)
            posInChart.setX(255.);
        if (posInChart.y() < 0.)
            posInChart.setY(0.);
        if (posInChart.y() > 255.)
            posInChart.setY(255.);
        interactionScatter->insert(0, posInChart);

        updateLineFromScatterSeries();
        emit ScatterSelectedOrChanged(interactionScatter->at(0), interactionScatter->color());
    }
    void mouseReleaseEvent(QMouseEvent *e) override {
        if (e->button() != Qt::MouseButton::LeftButton)
            return;

        if (!interactionScatter)
            return;

        updateDataFromInteraction();
        selectedScalar = interactionScalar;

        interactionScatter->setMarkerShape(
            QtCharts::QScatterSeries::MarkerShape::MarkerShapeCircle);
        interactionScatter = nullptr;

        updateSeriesFromData();
        emit TransferFunctionChanged();
    }

    void keyPressEvent(QKeyEvent *e) override { interactionKey = e->key(); }
    void keyReleaseEvent(QKeyEvent *e) override {
        if (interactionKey == Qt::Key::Key_Delete && selectedScatter && !interactionScatter) {
            unselect();

            auto del = tfScatters.at(selectedScalar);
            chart()->removeSeries(del);
            delete del;
            tfScatters.erase(selectedScalar);

            tfDat.DeletePoint(selectedScalar);
            updateSeriesFromData();
            emit TransferFunctionChanged();
        }

        interactionKey = Qt::Key::Key_unknown;
    }

    void leaveEvent(QEvent *e) override {
        if (!interactionScatter)
            return;

        updateDataFromInteraction();
        interactionScatter->setMarkerShape(
            QtCharts::QScatterSeries::MarkerShape::MarkerShapeCircle);
        interactionScatter = nullptr;

        selectedScatter->setBorderColor(Qt::GlobalColor::white);
        selectedScatter = nullptr;

        updateSeriesFromData();
        emit TransferFunctionChanged();
    }

  private:
    int interactionKey = Qt::Key::Key_unknown;
    int8_t interactionDir;
    uint8_t interactionScalar;
    uint8_t selectedScalar;
    QtCharts::QScatterSeries *interactionScatter = nullptr;
    QtCharts::QScatterSeries *selectedScatter = nullptr;

    QtCharts::QAreaSeries *tfAreas;
    QtCharts::QLineSeries *tfLines;
    // Currently QScatterSeries doesn't support assigning differene color to individual point,
    // thus multiple QScatterSeries are needed.
    std::map<uint8_t, QtCharts::QScatterSeries *> tfScatters;

    TransferFunctionData tfDat;

    void initSeries() {
        chart()->removeAllSeries();
        tfScatters.clear();

        tfAreas = new QtCharts::QAreaSeries();
        tfLines = new QtCharts::QLineSeries();
        chart()->addSeries(tfLines);
        chart()->addSeries(tfAreas);

        selectedScatter = nullptr;
    }

    void selectFromInteraction() {
        if (!interactionScatter)
            return;

        selectedScatter = interactionScatter;
        selectedScatter->setBorderColor(Qt::GlobalColor::red);
        selectedScalar = interactionScalar;
    }
    void unselect() {
        if (!selectedScatter)
            return;

        selectedScatter->setBorderColor(Qt::GlobalColor::white);
        selectedScatter = nullptr;
    }

    void updateLineFromScatterSeries() {
        tfLines->clear();
        for (auto &scalar_scatter : tfScatters)
            tfLines->append(std::get<1>(scalar_scatter)->at(0));
    }

    void updateSeriesFromData() {
        tfLines->clear();

        QLinearGradient gradient(0., 0., 1., 0.);
        gradient.setCoordinateMode(QGradient::CoordinateMode::ObjectBoundingMode);
        for (auto &scalar_rgba : tfDat.GetPoints()) {
            auto scalar = std::get<0>(scalar_rgba);
            auto rgba = std::get<1>(scalar_rgba);
            auto pnt = QPointF(scalar, 255. * rgba[3]);

            rgba[3] = 1.f;
            auto color = RGBAToQColor(rgba);

            auto itr = tfScatters.find(scalar);
            auto scatter = [&]() {
                if (itr == tfScatters.end()) {
                    auto itr_inserted =
                        tfScatters.emplace(std::make_pair(scalar, new QtCharts::QScatterSeries()));
                    auto scatter = std::get<0>(itr_inserted)->second;
                    chart()->addSeries(scatter);

                    return scatter;
                } else {
                    auto scatter = itr->second;
                    scatter->clear();

                    return scatter;
                }
            }();

            scatter->append(pnt);
            scatter->setColor(color);
            gradient.setColorAt(scalar / 255., color);

            tfLines->append(pnt);
        }

        tfAreas->setUpperSeries(tfLines);
        tfAreas->setBrush(gradient);

        chart()->createDefaultAxes();
        chart()->axes(Qt::Horizontal).first()->setRange(0, 255);
        chart()->axes(Qt::Vertical).first()->setRange(0, 255);
    }

    void updateDataFromInteraction() {
        uint8_t newScalar = std::round(interactionScatter->at(0).x());
        auto swap = [&]() {
            auto rgba = QColorToRGBA(interactionScatter->color());
            rgba[3] = std::max(std::min(interactionScatter->at(0).y() / 255., 1.), 0.);
            tfDat.ReplaceOrSetPoint(interactionScalar, newScalar, rgba);

            tfScatters.erase(interactionScalar);
            interactionScalar = newScalar;
            tfScatters.emplace(std::make_pair(interactionScalar, interactionScatter));
        };
        auto reset = [&]() {
            auto pnt = interactionScatter->at(0);
            auto rgba = QColorToRGBA(interactionScatter->color());
            rgba[3] = std::max(std::min(pnt.y() / 255., 1.), 0.);
            tfDat.ReplaceOrSetPoint(interactionScalar, interactionScalar, rgba);

            pnt.setX(interactionScalar);
            interactionScatter->insert(0, pnt);
        };

        if (interactionScalar != 0 && interactionScalar != 255)
            while (newScalar != interactionScalar && ((interactionDir < 0 && newScalar != 255) ||
                                                      (interactionDir > 0 && newScalar != 0))) {
                auto itr = tfScatters.find(newScalar);
                if (itr == tfScatters.end()) {
                    swap();
                    return;
                }
                newScalar -= interactionDir;
            }

        reset();
    }
};

class TransferFunctionEditor : public QWidget {
    Q_OBJECT

  public:
    static constexpr auto DefaultPresetIdx = static_cast<int>(QGradient::Preset::CloudyApple);

    TransferFunctionEditor(QWidget *parent = nullptr) : QWidget(parent) {
        chartView = new TransferFunctionChartView();
        chartView->setMinimumHeight(512);

        pushButton_colorDiag = new QPushButton(tr("Color"));
        label_scatterStatus = new QLabel();
        label_scatterStatus->setSizePolicy(
            QSizePolicy(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Minimum));
        label_note = new QLabel(tr("Add a TF point: Click Left while pressing [Ctrl]\nDelete a TF "
                                   "point: Select then press [Del]\n"));
        initComboBoxPresets();

        setLayout(new QGridLayout());
        auto gridLayout = reinterpret_cast<QGridLayout *>(layout());
        gridLayout->addWidget(chartView, 0, 0, 4, 4);
        gridLayout->addWidget(comboBox_tfPresets, 4, 0, 1, 4);
        gridLayout->addWidget(label_note, 5, 0, 1, 2);
        gridLayout->addWidget(pushButton_colorDiag, 5, 2, 1, 1);
        gridLayout->addWidget(label_scatterStatus, 5, 3, 1, 1);

        connect(chartView, &TransferFunctionChartView::TransferFunctionChanged, this,
                &TransferFunctionEditor::TransferFunctionChanged);
        connect(chartView, &TransferFunctionChartView::ScatterSelectedOrChanged,
                [&](const QPointF &point, const QColor &color) {
                    label_scatterStatus->setText(QString("scalar:%0, rgba: (%1, %2, %3, %4)")
                                                     .arg(static_cast<int>(std::round(point.x())))
                                                     .arg(color.red())
                                                     .arg(color.green())
                                                     .arg(color.blue())
                                                     .arg(static_cast<int>(std::round(point.y()))));

                    if (comboBox_tfPresets->currentIndex() != 0)
                        comboBox_tfPresets->setCurrentIndex(0);
                });
        connect(chartView, &TransferFunctionChartView::ScatterUnselected,
                [&]() { label_scatterStatus->clear(); });

        auto switchToPreset = [&](int idx) {
            if (idx == 0)
                return;

            chartView->SetFromGradientPreset(static_cast<QGradient::Preset>(idx));
            emit TransferFunctionChanged();
        };
        connect(comboBox_tfPresets, QOverload<int>::of(&QComboBox::currentIndexChanged),
                switchToPreset);
        switchToPreset(DefaultPresetIdx);

        connect(pushButton_colorDiag, &QPushButton::clicked, [&]() {
            auto oldColor = chartView->GetSelectedScatterColor();
            if (!oldColor.ok)
                return;

            auto newColor =
                QColorDialog::getColor(oldColor.val, this, tr("Pick a new RGBA for TF point"));
            if (!newColor.isValid())
                return;

            newColor.setAlpha(oldColor.val.alpha());
            chartView->SetSelectedScatterColor(newColor);
        });
    }

    const TransferFunctionData &GetTransferFunctionData() const {
        return chartView->GetTransferFunctionData();
    }
    void SetTransferFunctionData(const TransferFunctionData &tfDat) {
        chartView->SetTransferFunctionData(tfDat);

        comboBox_tfPresets->setCurrentIndex(0);
        emit TransferFunctionChanged();
    }

  Q_SIGNALS:
    void TransferFunctionChanged();

  private:
    QComboBox *comboBox_tfPresets;
    TransferFunctionChartView *chartView;
    QPushButton *pushButton_colorDiag;
    QLabel *label_note;
    QLabel *label_scatterStatus;

    void initComboBoxPresets() {
        constexpr std::array<int, 2> PixmapSz = {256, 32};
        comboBox_tfPresets = new QComboBox();
        comboBox_tfPresets->setIconSize(QSize(PixmapSz[0], PixmapSz[1]));
        comboBox_tfPresets->addItem(
            QApplication::style()->standardIcon(QStyle::StandardPixmap::SP_CustomBase),
            tr("Custom"));

        auto metaEnum = QMetaEnum::fromType<QGradient::Preset>();
        QPixmap pixmap(PixmapSz[0], PixmapSz[1]);
        QPainter painter(&pixmap);
        QLinearGradient lnGrad(0., 0., 1., 0.);
        lnGrad.setCoordinateMode(QGradient::CoordinateMode::ObjectBoundingMode);
        for (int i = 1; i <= metaEnum.keyCount(); ++i) {
            QGradient gradient(static_cast<QGradient::Preset>(i));
            int j = 0;
            for (auto &stop : gradient.stops()) {
                lnGrad.setColorAt(j / (gradient.stops().size() - 1), stop.second);
                ++j;
            }

            painter.setBrush(QBrush(lnGrad));
            painter.drawRect(0, 0, PixmapSz[0], PixmapSz[1]);

            comboBox_tfPresets->addItem(QIcon(pixmap), QString::fromLatin1(metaEnum.valueToKey(i)));
        }

        comboBox_tfPresets->setCurrentIndex(DefaultPresetIdx);
    }
};

} // namespace VIS4Earth

#endif // !VIS4EARTH_TF_CMPT_H
