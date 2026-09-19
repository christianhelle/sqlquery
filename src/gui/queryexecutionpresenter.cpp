#include "queryexecutionpresenter.h"

#include <QElapsedTimer>

QueryExecutionPresenter::QueryExecutionPresenter(QWidget *parent, QueryExecutor *executor)
    : executor(executor),
      presenter(std::make_unique<QueryResultPresenter>(parent)) {
}

ScriptOutcome QueryExecutionPresenter::run(const QString &script) {
    ScriptOutcome outcome;

    // Read either side of the run rather than reading the statements: a CREATE
    // inside a transaction that rolls back changes nothing, and an ALTER
    // changes the Schema without saying so in a word this could match on.
    const QString before = executor->schemaFingerprint();

    QElapsedTimer time;
    time.start();
    presenter->present(executor->runScriptPaged(script, &outcome.errors));
    outcome.elapsedMs = time.elapsed();

    const QString after = executor->schemaFingerprint();
    outcome.schemaChanged = !before.isEmpty() && !after.isEmpty() && before != after;

    return outcome;
}

void QueryExecutionPresenter::clearResults() {
    presenter->clear();
}

void QueryExecutionPresenter::presentToView(QTableView *view, QAbstractItemModel *model) {
    presenter->presentToView(view, model);
}

const QList<QTableView *> &QueryExecutionPresenter::resultViews() const {
    return presenter->views();
}
